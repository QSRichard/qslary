#include "http.h"
#include "qslary/http/httpclient_parser.h"
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <istream>
#include <sstream>
#include <string.h>

namespace qslary
{
namespace http
{

HttpMethod StringToHttpMethod(const std::string& method)
{
#define XX(num, name, string)                                                                                          \
  if (strncmp(#name, method.c_str(), strlen(#name)) == 0)                                                              \
    return HttpMethod::name;
  HTTP_METHOD_MAP(XX)
#undef XX
  return HttpMethod::UNKNOW;
}

HttpMethod CharsToHttpMethod(const char* method)
{
#define XX(num, name, string)                                                                                          \
  if (std::strncmp(#name, method, strlen(#name)) == 0)                                                                 \
    return HttpMethod::name;
  HTTP_METHOD_MAP(XX)
#undef XX
  return HttpMethod::UNKNOW;
}

HttpStatus StringToHttpStatus(const std::string& status)
{
#define XX(num, name, string)                                                                                          \
  if (strcmp(#name, status.c_str()) == 0)                                                                              \
    return HttpStatus::name;
  HTTP_STATUS_MAP(XX)
#undef XX
  return HttpStatus::UNKNOW;
}

HttpStatus CharsToHttpStatus(const char* status)
{
#define XX(num, name, string)                                                                                          \
  if (strcmp(#name, status) == 0)                                                                                      \
    return HttpStatus::name;
  HTTP_STATUS_MAP(XX)
#undef XX
  return HttpStatus::UNKNOW;
}

const char* HttpMethodToString(const HttpMethod& method)
{
  switch (method)
  {
#define XX(num, name, string)                                                                                          \
  case HttpMethod::name:                                                                                               \
    return #name;
    HTTP_METHOD_MAP(XX)
#undef XX
  default:
    return "UNKNOW";
  }
}

const char* HttpStatusToString(const HttpStatus& status)
{
  switch (status)
  {
#define XX(num, name, string)                                                                                          \
  case HttpStatus::name:                                                                                               \
    return #name;
    HTTP_STATUS_MAP(XX)
#undef XX
  default:
    return "UNKNOW";
  }
}

bool CaseInsenitiveless::operator()(const std::string& lhs, const std::string& rhs) const
{
  return strcasecmp(lhs.c_str(), rhs.c_str()) < 0;
}

HttpRequest::HttpRequest() : method_(HttpMethod::GET), status_(HttpStatus::OK), version_(0x11), close_(true), path_("/")
{
}

HttpRequest::HttpRequest(const HttpMethod& method, const HttpStatus& status, uint8_t version, bool close)
    : method_(method), status_(status), version_(version), close_(close)
{
}

std::string HttpRequest::getHeaderEntry(const std::string& key)
{
  auto it = headres_.find(key);
  if (it == headres_.end())
    return "";
  return it->second;
}

std::string HttpRequest::getParmaEntry(const std::string& key)
{
  auto it = parmas_.find(key);
  if (it == parmas_.end())
    return "";
  return it->second;
}

std::string HttpRequest::getCookisEntry(const std::string& key)
{
  auto it = cookies_.find(key);
  if (it == cookies_.end())
    return "";
  return it->second;
}

void HttpRequest::setHeaderEntry(const std::string& key, const std::string& value) { headres_[key] = value; }

void HttpRequest::setParmaEntry(const std::string& key, const std::string& value) { parmas_[key] = value; }

void HttpRequest::setCookiesEntry(const std::string& key, const std::string& value) { cookies_[key] = value; }

void HttpRequest::delHeaderEntry(const std::string& key)
{
  auto it = headres_.find(key);
  if (it != headres_.end())
    headres_.erase(it);
}

void HttpRequest::delParmaEntry(const std::string& key)
{
  auto it = parmas_.find(key);
  if (it != parmas_.end())
    parmas_.erase(it);
}

void HttpRequest::delCookiesEntry(const std::string& key)
{
  auto it = cookies_.find(key);
  if (it != cookies_.end())
    cookies_.erase(it);
}

std::ostream& HttpRequest::dump(std::ostream& ios) const
{
  ios << HttpMethodToString(method_) << " " << path_ << (query_.empty() ? "" : "?") << query_
      << (fragment_.empty() ? "" : "#") << fragment_ << " "
      << "HTTP/" << (uint32_t)(version_ >> 4) << "." << (uint32_t)(version_ & 0x0f) << "\r\n";
  // ios << "connection: " << ((close_) ? "close" : "keep-alive") << "\r\n";

  if(!headres_.empty())
    ios << "headers"<<headres_.size()
        << "\r\n";
  
  for (auto it : headres_)
  {
    if (it.first != "connection")
      ios << it.first << ": " << it.second << "\r\n";
  }
  if (!body_.empty())
    ios << "content-length: " << body_.size() << "\r\n\r\n" << body_;
  else
    ios << "\r\n";
  return ios;
}

std::string HttpRequest::toString() const
{
  std::stringstream ss;
  dump(ss);
  return ss.str();
}

HttpResponse::HttpResponse() : status_(HttpStatus::OK), version_(0x11), close_(true) {}

HttpResponse::HttpResponse(const HttpStatus& status, uint8_t version, bool close)
    : status_(status), version_(version), close_(close)
{
}

std::string HttpResponse::getHeaderEntry(const std::string& key)
{
  auto it = headers_.find(key);
  if (it == headers_.end())
    return "";
  return it->second;
}
void HttpResponse::setHeaderEntry(const std::string& key, const std::string& value) { headers_[key] = value; }
void HttpResponse::delHeaderEntry(const std::string& key)
{
  auto it = headers_.find(key);
  if (it != headers_.end())
    headers_.erase(it);
}

std::ostream& HttpResponse::dump(std::ostream& ios) const
{

  ios << "HTTP/" << (uint32_t)(version_ >> 4) << "." << (uint32_t)(version_ & 0x0f) << " " << (uint32_t)status_ << " "
      << (reason_.empty() ? HttpStatusToString(status_) : reason_) << "\r\n";

  for (auto& it : headers_)
  {
    if (it.first != "connection")
      ios << it.first << ": " << it.second << "\r\n";
  }

  ios << "connection: " << (close_ ? "close" : "keep-alive") << "\r\n";
  if (!body_.empty())
    ios << "content-length: " << body_.size() << "\r\n\r\n" << body_ << "\r\n";
  else
    ios << "\r\n";
  return ios;
}

void HttpRequest::init()
{
  std::string conn;
  getHeaderEntryAs("connection", conn);
  if (!conn.empty())
  {
    if (strcasecmp(conn.c_str(), "keep-alive") == 0)
    {
      close_ = false;
    }
    else
    {
      close_ = true;
    }
  }
}

std::string HttpResponse::toString() const
{
  std::stringstream ss;
  dump(ss);
  return ss.str();
}















void on_request_method(void* data, const char* at, size_t lenth)
{
  HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
  HttpMethod method = CharsToHttpMethod(at);
  if (method == HttpMethod::UNKNOW)
  {
    // TODO logging
    // invalid method
    std::cout << "HTTP method invalid" << std::endl;

    parser->setError(1000);
  }
  parser->getData()->setMethod(method);
}

void on_request_uri(void* data, const char* at, size_t lenth) {}

void on_request_fragment(void* data, const char* at, size_t lenth)
{
  HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
  parser->getData()->setFragment(std::string(at, lenth));
}

void on_request_path(void* data, const char* at, size_t lenth)
{
  HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
  parser->getData()->setPath(std::string(at, lenth));
}

void on_request_query(void* data, const char* at, size_t lenth)
{
  HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
  parser->getData()->setQuery(std::string(at, lenth));
}

void on_request_version(void* data, const char* at, size_t lenth)
{
  HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
  if (strncmp(at, "HTTP/1.1", lenth) == 0)
    parser->getData()->setVersion(0x11);
  else if (strncmp(at, "HTTP/1.0", lenth) == 0)
    parser->getData()->setVersion(0x10);
  else
  {
    // TODO logging
    // Invalid Http Version
    std::cout << "    parser->setError(1001);" << std::endl;
    parser->setError(1001);
  }
}

void on_request_header_done(void* data, const char* at, size_t lenth) {}

void on_request_http_field(void* data, const char* field, size_t flen, const char* value, size_t vlen)
{
  HttpRequestParser* parser = static_cast<HttpRequestParser*>(data);
  if (flen == 0)
  {
    // TODO logging
    // field length = 0
    parser->setError(1002);
    return;
  }
  parser->getData()->setHeaderEntry(std::string(field, flen), std::string(value, vlen));
}


HttpRequestParser::HttpRequestParser()
{
  requestData_.reset(new qslary::http::HttpRequest);
  http_parser_init(&parser_);
  parser_.request_method = on_request_method;
  parser_.request_uri = on_request_uri;
  parser_.fragment = on_request_fragment;
  parser_.request_path = on_request_path;
  parser_.query_string = on_request_query;
  parser_.http_version = on_request_version;
  parser_.header_done = on_request_header_done;
  parser_.http_field = on_request_http_field;
  parser_.data = this;
  error_ = 0;
}

size_t HttpRequestParser::execute(char* data, size_t len)
{
  size_t ret = http_parser_execute(&parser_, data, len, 0);
  std::memmove(data, data + ret, len - ret);
  return ret;
}
int HttpRequestParser::isFinished() { return http_parser_finish(&parser_); }

int HttpRequestParser::hasError() { return error_ || http_parser_has_error(&parser_); }

uint64_t HttpRequestParser::getContentLength()
{
  uint64_t val = 0;
  requestData_->getHeaderEntryAs("content-length", val);
  return val;
}

















void on_response_reason(void* data, const char* at, size_t lenth)
{
  HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
  parser->getData()->setReason(std::string(at, lenth));
}
void on_response_status(void* data, const char* at, size_t lenth)
{
  HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
  HttpStatus status = (HttpStatus)(atoi(at));
  parser->getData()->setStatus(status);
}
void on_response_chunk(void* data, const char* at, size_t lenth) {}
void on_response_version(void* data, const char* at, size_t lenth)
{
  HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
  if (strncmp(at, "HTTP/1.1", lenth) == 0)
  {
    parser->getData()->setVersion(0x11);
    return;
  }
  if (strncmp(at, "HTTP/1.0", lenth))
  {
    parser->getData()->setVersion(0x10);
    return;
  }
  // TODO logging
  // Http Version error 1001
  parser->setError(1001);
}

void on_response_header_done(void* data, const char* at, size_t lenth) {}
void on_response_last_chunk(void* data, const char* at, size_t lenth) {}

void on_response_field(void* data, const char* field, size_t flen, const char* value, size_t vlen)
{
  HttpResponseParser* parser = static_cast<HttpResponseParser*>(data);
  if (flen == 0)
  {
    std::cout << "invalid http response field length == 0" << std::endl;
        // TODO logging
        // field length = 0
        // parser->setError(1002);
        return;
  }
  parser->getData()->setHeaderEntry(std::string(field, flen), std::string(value, vlen));
}

HttpResponseParser::HttpResponseParser()
{
  responseData_.reset(new qslary::http::HttpResponse);
  httpclient_parser_init(&parser_);
  parser_.reason_phrase = on_response_reason;
  parser_.status_code = on_response_status;
  parser_.chunk_size = on_response_chunk;
  parser_.http_version = on_response_version;
  parser_.header_done = on_response_header_done;
  parser_.last_chunk = on_response_last_chunk;
  parser_.http_field = on_response_field;
  parser_.data = this;
  error_ = 0;
}

size_t HttpResponseParser::execute(char* data, size_t len,bool chunck)
{
  if (chunck)
  {
    httpclient_parser_init(&parser_);
  }
  size_t ret = httpclient_parser_execute(&parser_, data, len, 0);
  std::memmove(data, data + ret, len - ret);
  return ret;
}

int HttpResponseParser::isFinished() { return httpclient_parser_finish(&parser_); }
int HttpResponseParser::hasError() { return error_ || httpclient_parser_has_error(&parser_); }

uint64_t HttpResponseParser::getContentLength()
{
  uint64_t val = 0;
  responseData_->getHeaderEntryAs("content-length", val);
  return val;
}

std::ostream& operator<<(std::ostream& os, const HttpRequest& req) { return req.dump(os); }

std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp) { return rsp.dump(os); }

} // namespace http
} // namespace qslary

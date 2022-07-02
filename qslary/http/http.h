#ifndef __QSLARY_HTTP_H_
#define __QSLARY_HTTP_H_

#include "qslary/base/types.h"
#include "qslary/base/noncopyable.h"
#include "qslary/http/http11_parser.h"
#include "qslary/http/httpclient_parser.h"

#include <boost/lexical_cast.hpp>
#include <cstddef>
#include <cstdint>
#include <istream>
#include <map>
#include <iostream>
#include <memory>
#include <string>
namespace qslary
{
namespace http
{

#define HTTP_STATUS_MAP(XX)                                                                                            \
  XX(100, CONTINUE, Continue)                                                                                          \
  XX(101, SWITCHING_PROTOCOLS, Switching Protocols)                                                                    \
  XX(102, PROCESSING, Processing)                                                                                      \
  XX(200, OK, OK)                                                                                                      \
  XX(201, CREATED, Created)                                                                                            \
  XX(202, ACCEPTED, Accepted)                                                                                          \
  XX(203, NON_AUTHORITATIVE_INFORMATION, Non - Authoritative Information)                                              \
  XX(204, NO_CONTENT, No Content)                                                                                      \
  XX(205, RESET_CONTENT, Reset Content)                                                                                \
  XX(206, PARTIAL_CONTENT, Partial Content)                                                                            \
  XX(207, MULTI_STATUS, Multi - Status)                                                                                \
  XX(208, ALREADY_REPORTED, Already Reported)                                                                          \
  XX(226, IM_USED, IM Used)                                                                                            \
  XX(300, MULTIPLE_CHOICES, Multiple Choices)                                                                          \
  XX(301, MOVED_PERMANENTLY, Moved Permanently)                                                                        \
  XX(302, FOUND, Found)                                                                                                \
  XX(303, SEE_OTHER, See Other)                                                                                        \
  XX(304, NOT_MODIFIED, Not Modified)                                                                                  \
  XX(305, USE_PROXY, Use Proxy)                                                                                        \
  XX(307, TEMPORARY_REDIRECT, Temporary Redirect)                                                                      \
  XX(308, PERMANENT_REDIRECT, Permanent Redirect)                                                                      \
  XX(400, BAD_REQUEST, Bad Request)                                                                                    \
  XX(401, UNAUTHORIZED, Unauthorized)                                                                                  \
  XX(402, PAYMENT_REQUIRED, Payment Required)                                                                          \
  XX(403, FORBIDDEN, Forbidden)                                                                                        \
  XX(404, NOT_FOUND, Not Found)                                                                                        \
  XX(405, METHOD_NOT_ALLOWED, Method Not Allowed)                                                                      \
  XX(406, NOT_ACCEPTABLE, Not Acceptable)                                                                              \
  XX(407, PROXY_AUTHENTICATION_REQUIRED, Proxy Authentication Required)                                                \
  XX(408, REQUEST_TIMEOUT, Request Timeout)                                                                            \
  XX(409, CONFLICT, Conflict)                                                                                          \
  XX(410, GONE, Gone)                                                                                                  \
  XX(411, LENGTH_REQUIRED, Length Required)                                                                            \
  XX(412, PRECONDITION_FAILED, Precondition Failed)                                                                    \
  XX(413, PAYLOAD_TOO_LARGE, Payload Too Large)                                                                        \
  XX(414, URI_TOO_LONG, URI Too Long)                                                                                  \
  XX(415, UNSUPPORTED_MEDIA_TYPE, Unsupported Media Type)                                                              \
  XX(416, RANGE_NOT_SATISFIABLE, Range Not Satisfiable)                                                                \
  XX(417, EXPECTATION_FAILED, Expectation Failed)                                                                      \
  XX(421, MISDIRECTED_REQUEST, Misdirected Request)                                                                    \
  XX(422, UNPROCESSABLE_ENTITY, Unprocessable Entity)                                                                  \
  XX(423, LOCKED, Locked)                                                                                              \
  XX(424, FAILED_DEPENDENCY, Failed Dependency)                                                                        \
  XX(426, UPGRADE_REQUIRED, Upgrade Required)                                                                          \
  XX(428, PRECONDITION_REQUIRED, Precondition Required)                                                                \
  XX(429, TOO_MANY_REQUESTS, Too Many Requests)                                                                        \
  XX(431, REQUEST_HEADER_FIELDS_TOO_LARGE, Request Header Fields Too Large)                                            \
  XX(451, UNAVAILABLE_FOR_LEGAL_REASONS, Unavailable For Legal Reasons)                                                \
  XX(500, INTERNAL_SERVER_ERROR, Internal Server Error)                                                                \
  XX(501, NOT_IMPLEMENTED, Not Implemented)                                                                            \
  XX(502, BAD_GATEWAY, Bad Gateway)                                                                                    \
  XX(503, SERVICE_UNAVAILABLE, Service Unavailable)                                                                    \
  XX(504, GATEWAY_TIMEOUT, Gateway Timeout)                                                                            \
  XX(505, HTTP_VERSION_NOT_SUPPORTED, HTTP Version Not Supported)                                                      \
  XX(506, VARIANT_ALSO_NEGOTIATES, Variant Also Negotiates)                                                            \
  XX(507, INSUFFICIENT_STORAGE, Insufficient Storage)                                                                  \
  XX(508, LOOP_DETECTED, Loop Detected)                                                                                \
  XX(510, NOT_EXTENDED, Not Extended)                                                                                  \
  XX(511, NETWORK_AUTHENTICATION_REQUIRED, Network Authentication Required)

enum class HttpStatus
{
#define XX(num, name, string) name = num,
  HTTP_STATUS_MAP(XX)
#undef XX
  UNKNOW,
};

/* Request Methods */
#define HTTP_METHOD_MAP(XX)                                                                                            \
  XX(0, DELETE, DELETE)                                                                                                \
  XX(1, GET, GET)                                                                                                      \
  XX(2, HEAD, HEAD)                                                                                                    \
  XX(3, POST, POST)                                                                                                    \
  XX(4, PUT, PUT)                                                                                                      \
  /* pathological */                                                                                                   \
  XX(5, CONNECT, CONNECT)                                                                                              \
  XX(6, OPTIONS, OPTIONS)                                                                                              \
  XX(7, TRACE, TRACE)                                                                                                  \
  /* WebDAV */                                                                                                         \
  XX(8, COPY, COPY)                                                                                                    \
  XX(9, LOCK, LOCK)                                                                                                    \
  XX(10, MKCOL, MKCOL)                                                                                                 \
  XX(11, MOVE, MOVE)                                                                                                   \
  XX(12, PROPFIND, PROPFIND)                                                                                           \
  XX(13, PROPPATCH, PROPPATCH)                                                                                         \
  XX(14, SEARCH, SEARCH)                                                                                               \
  XX(15, UNLOCK, UNLOCK)                                                                                               \
  XX(16, BIND, BIND)                                                                                                   \
  XX(17, REBIND, REBIND)                                                                                               \
  XX(18, UNBIND, UNBIND)                                                                                               \
  XX(19, ACL, ACL)                                                                                                     \
  /* subversion */                                                                                                     \
  XX(20, REPORT, REPORT)                                                                                               \
  XX(21, MKACTIVITY, MKACTIVITY)                                                                                       \
  XX(22, CHECKOUT, CHECKOUT)                                                                                           \
  XX(23, MERGE, MERGE)                                                                                                 \
  /* upnp */                                                                                                           \
  XX(24, MSEARCH, M - SEARCH)                                                                                          \
  XX(25, NOTIFY, NOTIFY)                                                                                               \
  XX(26, SUBSCRIBE, SUBSCRIBE)                                                                                         \
  XX(27, UNSUBSCRIBE, UNSUBSCRIBE)                                                                                     \
  /* RFC-5789 */                                                                                                       \
  XX(28, PATCH, PATCH)                                                                                                 \
  XX(29, PURGE, PURGE)                                                                                                 \
  /* CalDAV */                                                                                                         \
  XX(30, MKCALENDAR, MKCALENDAR)                                                                                       \
  /* RFC-2068, section 19.6.1.2 */                                                                                     \
  XX(31, LINK, LINK)                                                                                                   \
  XX(32, UNLINK, UNLINK)                                                                                               \
  /* icecast */                                                                                                        \
  XX(33, SOURCE, SOURCE)

enum class HttpMethod
{
#define XX(num, name, string) name = num,
  HTTP_METHOD_MAP(XX)
#undef XX
  UNKNOW,
};


HttpMethod StringToHttpMethod(const std::string& method);
HttpMethod CharsToHttpMethod(const char* method);

HttpStatus StringToHttpStatus(const std::string& status);
HttpStatus CharsToHttpStatus(const char* status);


const char* HttpMethodToString(const HttpMethod& method);
const char* HttpStatusToString(const HttpStatus& status);

struct CaseInsenitiveless
{
  bool operator()(const std::string& lhs, const std::string& rhs) const;
};

class HttpRequest:public noncopyable
{
public:
  typedef std::shared_ptr<HttpRequest> ptr;
  typedef std::map<std::string, std::string, CaseInsenitiveless> HttpRequestMap;
  std::ostream& dump(std::ostream& ios) const;
  std::string toString() const;

  HttpRequest();
  HttpRequest(const HttpMethod& method, const HttpStatus& status, uint8_t version=0x11, bool close=true);
  ~HttpRequest()
  {
  };

  void init();


  HttpMethod getMethod() const { return method_; }
  HttpStatus getStatus() const { return status_; }
  uint8_t getVersion() const { return version_; }
  bool isClose() const { return close_; }
  std::string getPath() const { return path_; }
  std::string getQuery() const { return query_; }
  std::string getFragment() const { return fragment_; }
  std::string getBody() const { return body_; }


  void setMethod(const HttpMethod& method) { method_ = method; }
  void setStatus(const HttpStatus& status) { status_ = status; }
  void setVersion(uint8_t version) { version_ = version; }
  void setClose(bool close) { close_ = close; }
  void setPath(const std::string& path) { path_ = path; };
  void setQuery(const std::string& query) { query_ = query; }
  void setFragment(const std::string& framgent) { fragment_ = framgent; }
  void setBody(const std::string& body) { body_ = body; }

  const HttpRequestMap& getHeaders() const { return headres_; }
  const HttpRequestMap& getParmas() const { return parmas_; }
  const HttpRequestMap& getCookis() const { return cookies_; }

  std::string getHeaderEntry(const std::string& key);
  std::string getParmaEntry(const std::string& key);
  std::string getCookisEntry(const std::string& key);

  void setHeaderEntry(const std::string& key, const std::string& value);
  void setParmaEntry(const std::string& key, const std::string& value);
  void setCookiesEntry(const std::string& key, const std::string& value);

  void delHeaderEntry(const std::string& key);
  void delParmaEntry(const std::string& key);
  void delCookiesEntry(const std::string& key);


  template <typename T>
  bool getHeaderEntryAs(const std::string& key, T& value)
  {
    return getAs(headres_,key,value);
  }
  template <typename T>
  bool getParmaEntryAs(const std::string& key, T& value)
  {
    return getAs(parmas_,key,value);
  }

  template <typename T>
  bool getCookieEntryAs(const std::string& key, T& value)
  {
    return getAs(cookies_, key, value);
  }

private:
  template <typename T>
  bool getAs(const HttpRequestMap& map, const std::string& key,T& value)
  {
    auto it = map.find(key);
    if (it == map.end())
      return false;
    try
    {
      value = boost::lexical_cast<T>(it->second);
      return true;
    }
    catch (...)
    {
      return false;
    }
  }

private :
  HttpMethod method_;
  HttpStatus status_;
  uint8_t version_;
  bool close_;
  std::string path_;
  std::string query_;
  std::string fragment_;
  std::string body_;

  HttpRequestMap headres_;
  HttpRequestMap parmas_;
  HttpRequestMap cookies_;
  
};




















class HttpResponse:public noncopyable
{
public:
  typedef std::shared_ptr<HttpResponse> ptr;
  typedef std::map<std::string, std::string, CaseInsenitiveless> HttpResponseMap;
  std::ostream& dump(std::ostream& ios) const;
  std::string toString() const;
  
  HttpResponse();
  HttpResponse(const HttpStatus& status, uint8_t version = 0x11, bool close = true);
  ~HttpResponse() = default;
  
  HttpStatus getStatus() const { return status_; }
  uint8_t getVersion() const { return version_; }
  bool getClose() const { return close_; }
  std::string getBody() const { return body_; }
  std::string getReason() const { return reason_; }
  const HttpResponseMap& getHeaders() const { return headers_; }

  void setStatus(const HttpStatus& status) { status_ = status; }
  void setVersion(uint8_t version) { version_ = version; }
  void setClose(bool close) { close_ = close; }
  void setBody(const std::string& body) { body_ = body; }
  void setReason(const std::string& reason) { reason_ = reason; }
  void setHeaders(const HttpResponseMap& headers) { headers_ = headers; }

  std::string getHeaderEntry(const std::string& key);
  void setHeaderEntry(const std::string& key, const std::string& value);
  void delHeaderEntry(const std::string& key);

  template <typename T>
  bool getHeaderEntryAs(const std::string& key, T& value)
  {
    return getAs(headers_, key, value);
  }

private:
  template <typename T>
  bool getAs(const HttpResponseMap& map, const std::string& key, T& value)
  {
    auto it = map.find(key);
    if (it == map.end())
      return false;
    try
    {
      value = boost::lexical_cast<T>(it->second);
      return true;
    }
    catch (...)
    {
      return false;
    }
  }

private:
  HttpStatus status_;
  uint8_t version_;
  bool close_;
  std::string body_;
  std::string reason_;
  HttpResponseMap headers_;
};


class HttpRequestParser
{
public:
  typedef std::shared_ptr<HttpRequestParser> ptr;
  HttpRequestParser();

  size_t execute(char* data, size_t len);
  int isFinished();
  int hasError();

  HttpRequest::ptr getData() const { return requestData_; }
  void setError(int error) { error_ = error; }

  const http_parser& getParser() const
  {
    return parser_;
  }

  uint64_t getContentLength();

private:
  struct http_parser parser_;
  int error_;
  HttpRequest::ptr requestData_;
};

class HttpResponseParser
{
public:
  typedef std::shared_ptr<HttpResponseParser> ptr;
  HttpResponseParser();

  size_t execute(char* data, size_t len,bool chunck=false);
  int isFinished();
  int hasError();

  int getError() const { return error_; }
  uint64_t getContentLength();

  void setError(int error) { error_ = error; }
  HttpResponse::ptr getData() const { return responseData_; }

  const httpclient_parser& getParser() const { return parser_; }
  
  
private:
  
  struct httpclient_parser parser_;
  int error_;
  HttpResponse::ptr responseData_;
};

std::ostream& operator<<(std::ostream& os, const HttpRequest& req);

std::ostream& operator<<(std::ostream& os, const HttpResponse& rsp);


} // namespace http

} // namespace qslary


#endif
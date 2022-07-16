#include "Http.h"
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

HttpMethod StringToHttpMethod(const std::string &method)
{
#define XX(num, name, string)                                                  \
    if (strncmp(#name, method.c_str(), strlen(#name)) == 0)                    \
        return HttpMethod::name;
    HTTP_METHOD_MAP(XX)
#undef XX
    return HttpMethod::UNKNOW;
}

HttpMethod CharsToHttpMethod(const char *method)
{
#define XX(num, name, string)                                                  \
    if (std::strncmp(#name, method, strlen(#name)) == 0)                       \
        return HttpMethod::name;
    HTTP_METHOD_MAP(XX)
#undef XX
    return HttpMethod::UNKNOW;
}

HttpStatus StringToHttpStatus(const std::string &status)
{
#define XX(num, name, string)                                                  \
    if (strcmp(#name, status.c_str()) == 0)                                    \
        return HttpStatus::name;
    HTTP_STATUS_MAP(XX)
#undef XX
    return HttpStatus::UNKNOW;
}

HttpStatus CharsToHttpStatus(const char *status)
{
#define XX(num, name, string)                                                  \
    if (strcmp(#name, status) == 0)                                            \
        return HttpStatus::name;
    HTTP_STATUS_MAP(XX)
#undef XX
    return HttpStatus::UNKNOW;
}

const char *HttpMethodToString(const HttpMethod &method)
{
    switch (method)
    {
#define XX(num, name, string)                                                  \
    case HttpMethod::name:                                                     \
        return #name;
        HTTP_METHOD_MAP(XX)
#undef XX
    default:
        return "UNKNOW";
    }
}

const char *HttpStatusToString(const HttpStatus &status)
{
    switch (status)
    {
#define XX(num, name, string)                                                  \
    case HttpStatus::name:                                                     \
        return #name;
        HTTP_STATUS_MAP(XX)
#undef XX
    default:
        return "UNKNOW";
    }
}

HttpRequest::HttpRequest()
    : method_(HttpMethod::GET), status_(HttpStatus::OK), mVersion("1.1"),
      close_(true), path_("/")
{}

HttpRequest::HttpRequest(const HttpMethod &method, const HttpStatus &status,
                         const std::string &version, bool close)
    : method_(method), status_(status), mVersion(version), close_(close)
{}

std::string HttpRequest::getHeaderEntry(const std::string &key)
{
    auto it = headres_.find(key);
    if (it == headres_.end())
        return "";
    return it->second;
}

std::string HttpRequest::getParmaEntry(const std::string &key)
{
    auto it = parmas_.find(key);
    if (it == parmas_.end())
        return "";
    return it->second;
}

std::string HttpRequest::getCookisEntry(const std::string &key)
{
    auto it = cookies_.find(key);
    if (it == cookies_.end())
        return "";
    return it->second;
}

void HttpRequest::setHeaderEntry(const std::string &key,
                                 const std::string &value)
{
    headres_[key] = value;
}

void HttpRequest::setParmaEntry(const std::string &key,
                                const std::string &value)
{
    parmas_[key] = value;
}

void HttpRequest::setCookiesEntry(const std::string &key,
                                  const std::string &value)
{
    cookies_[key] = value;
}

void HttpRequest::delHeaderEntry(const std::string &key)
{
    auto it = headres_.find(key);
    if (it != headres_.end())
        headres_.erase(it);
}

void HttpRequest::delParmaEntry(const std::string &key)
{
    auto it = parmas_.find(key);
    if (it != parmas_.end())
        parmas_.erase(it);
}

void HttpRequest::delCookiesEntry(const std::string &key)
{
    auto it = cookies_.find(key);
    if (it != cookies_.end())
        cookies_.erase(it);
}

std::ostream &HttpRequest::dump(std::ostream &ios) const
{
    ios << HttpMethodToString(method_) << " " << mURL << " HTTP/" << mVersion
        << "\r\n";

    for (auto it : headres_)
    {
        if (it.first != "connection")
        {
            ios << it.first << ": " << it.second << "\r\n";
        }
    }
    if (!body_.empty())
    {
        ios << "content-length: " << body_.size() << "\r\n\r\n" << body_;
    }
    else
    {
        ios << "\r\n";
    }
    return ios;
}

std::string HttpRequest::toString() const
{
    std::stringstream ss;
    dump(ss);
    return ss.str();
}

HttpResponse::HttpResponse()
    : status_(HttpStatus::OK), mVersion("1.1"), close_(true)
{}

HttpResponse::HttpResponse(const HttpStatus &status, const std::string &version,
                           bool close)
    : status_(status), mVersion(version), close_(close)
{}

std::string HttpResponse::getHeaderEntry(const std::string &key)
{
    auto it = headers_.find(key);
    if (it == headers_.end())
        return "";
    return it->second;
}
void HttpResponse::setHeaderEntry(const std::string &key,
                                  const std::string &value)
{
    headers_[key] = value;
}
void HttpResponse::delHeaderEntry(const std::string &key)
{
    auto it = headers_.find(key);
    if (it != headers_.end())
        headers_.erase(it);
}

std::ostream &HttpResponse::dump(std::ostream &ios) const
{

    ios << "HTTP/" << mVersion << " " << (uint32_t)status_ << " "
        << (reason_.empty() ? HttpStatusToString(status_) : reason_) << "\r\n";

    for (auto &it : headers_)
    {
        if (it.first != "connection")
            ios << it.first << ": " << it.second << "\r\n";
    }

    ios << "connection: " << (close_ ? "close" : "keep-alive") << "\r\n";
    if (!body_.empty())
        ios << "content-length: " << body_.size() << "\r\n\r\n"
            << body_ << "\r\n";
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

std::ostream &operator<<(std::ostream &os, const HttpRequest &req)
{
    return req.dump(os);
}

std::ostream &operator<<(std::ostream &os, const HttpResponse &rsp)
{
    return rsp.dump(os);
}

} // namespace http
} // namespace qslary

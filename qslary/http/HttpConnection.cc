#include "qslary/http/HttpConnection.h"
#include "qslary/net/Address.h"
#include "qslary/base/Mutex.h"
#include "qslary/net/Socket.h"
#include "qslary/http/http.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <functional>
#include <memory>
#include <sstream>
#include <string>
#include <strings.h>
#include <unistd.h>
#include <vector>
#include "qslary/base/util.h"

namespace qslary
{
namespace http
{

std::string HttpResult::toString() const
{
  std::stringstream ss;
  ss << "[HttpResult result=" << result << " error=" << error
     << " response=" << (response ? response->toString() : "nullptr") << "]";
  return ss.str();
}

HttpConnection::HttpConnection(Socket::ptr sock, bool ower) : SockStream(sock, ower) {}

HttpResponse::ptr HttpConnection::recvResponse(HttpResponse::ptr& r)
{

  HttpResponseParser::ptr parser(new HttpResponseParser);

  int buff_size = 65532;

  std::shared_ptr<char> buffer(new char[buff_size + 1], [](char* ptr) { delete[] ptr; });
  char* data = buffer.get();
  int offset = 0;
  do
  {
    int len = read(data + offset, buff_size - offset);
    if (len <= 0)
    {
      close();
      return nullptr;
    }
    len += offset;
    data[len] = '\0';
    size_t nparse = parser->execute(data, len, false);
    if (parser->hasError())
    {
      close();
      return nullptr;
    }
    offset = len - nparse;
    if (offset == (int)buff_size)
    {
      close();
      return nullptr;
    }
    if (parser->isFinished())
    {
      break;
    }
  } while (true);
  auto& client_parser = parser->getParser();
  std::string body;
  if (client_parser.chunked)
  {
    int len = offset;
    do
    {
      bool begin = true;
      do
      {
        if (!begin || len == 0)
        {
          int rt = read(data + len, buff_size - len);
          if (rt <= 0)
          {
            close();
            return nullptr;
          }
          len += rt;
        }
        data[len] = '\0';
        size_t nparse = parser->execute(data, len, true);
        if (parser->hasError())
        {
          close();
          return nullptr;
        }
        len -= nparse;
        if (len == (int)buff_size)
        {
          close();
          return nullptr;
        }
        begin = false;
      } while (!parser->isFinished());
      // len -= 2;

      //   SYLAR_LOG_DEBUG(g_logger) << "content_len=" << client_parser.content_len;
      if (client_parser.content_len + 2 <= len)
      {
        body.append(data, client_parser.content_len);
        memmove(data, data + client_parser.content_len + 2, len - client_parser.content_len - 2);
        len -= client_parser.content_len + 2;
      }
      else
      {
        body.append(data, len);
        int left = client_parser.content_len - len + 2;
        while (left > 0)
        {
          int rt = read(data, left > (int)buff_size ? (int)buff_size : left);
          if (rt <= 0)
          {
            close();
            return nullptr;
          }
          body.append(data, rt);
          left -= rt;
        }
        body.resize(body.size() - 2);
        len = 0;
      }
    } while (!client_parser.chunks_done);
  }
  else
  {
    int64_t length = parser->getContentLength();
    if (length > 0)
    {
      body.resize(length);

      int len = 0;
      if (length >= offset)
      {
        memcpy(&body[0], data, offset);
        len = offset;
      }
      else
      {
        memcpy(&body[0], data, length);
        len = length;
      }
      length -= offset;
      if (length > 0)
      {
        if (readFixSize(&body[len], length) <= 0)
        {
          close();
          return nullptr;
        }
      }
    }
  }

  //   std::shared_ptr<char> buffer(new char[buff_size+1], [](char* ptr) { delete[] ptr; });
  //   char* data = buffer.get();
  //   int offset = 0;
  //   do
  //   {
  //     int len = read(data + offset, buff_size - offset);
  //     if (len <= 0)
  //     {
  //       close();
  //       return nullptr;
  //     }
  //     len += offset;
  //     data[len] = '\0';
  //     size_t nparse = parser->execute(data, len);
  //     std::cout << "nparse " << nparse << std::endl;
  //     if (parser->hasError())
  //     {
  //       close();
  //       return nullptr;
  //     }
  //     offset = len - nparse;
  //     if (offset == (int)buff_size)
  //     {
  //       close();
  //       return nullptr;
  //     }
  //     if (parser->isFinished())
  //     {
  //       break;
  //     }
  //   } while (true);

  //   auto client_parser = parser->getParser();

  //   std::string body;
  //   if (client_parser.chunked)
  //   {
  //     int len = offset;
  //     do
  //     {
  //       bool begin = true;
  //       do
  //       {
  //         if (!begin || len == 0)
  //         {
  //           int rt = read(data + len, buff_size - len);
  //           if (rt <= 0)
  //           {
  //             close();
  //             return nullptr;
  //           }
  //           len += rt;
  //         }
  //         data[len] = '\0';
  //         size_t nparse = parser->execute(data, len, true);
  //         if (parser->hasError())
  //         {
  //           close();
  //           return nullptr;
  //         }
  //         len -= nparse;
  //         if (len == (int)buff_size)
  //         {
  //           close();
  //           return nullptr;
  //         }
  //         begin = false;
  //         std::cout << "+++++++++++++++++++++++++++++++" << std::endl;
  //       } while (!parser->isFinished());
  //       // len -= 2;
  //       std::cout << "+++++++++++++++++++++++++++++++" << std::endl;
  //       //   SYLAR_LOG_DEBUG(g_logger) << "content_len=" << client_parser.content_len;
  //       if (client_parser.content_len + 2 <= len)
  //       {
  //         std::cout << "=============================" << std::endl;
  //         body.append(data, client_parser.content_len);
  //         std::cout << "=============================" << std::endl;
  //         memmove(data, data + client_parser.content_len + 2, len - client_parser.content_len - 2);
  //         len -= client_parser.content_len + 2;
  //         std::cout << "=============================" << std::endl;
  //       }
  //       else
  //       {
  //         std::cout << "++++-------------------------------++" << std::endl;
  //         body.append(data, len);
  //         int left = client_parser.content_len - len + 2;
  //         while (left > 0)
  //         {
  //           int rt = read(data, left > (int)buff_size ? (int)buff_size : left);
  //           if (rt <= 0)
  //           {
  //             close();
  //             return nullptr;
  //           }
  //           body.append(data, rt);
  //           left -= rt;
  //         }
  //         body.resize(body.size() - 2);
  //         len = 0;
  //       }
  //     } while (!client_parser.chunks_done);
  //   }
  //   else
  //   {
  //     int64_t length = parser->getContentLength();
  //     if (length > 0)
  //     {
  //       body.resize(length);

  //       int len = 0;
  //       if (length >= offset)
  //       {
  //         memcpy(&body[0], data, offset);
  //         len = offset;
  //       }
  //       else
  //       {
  //         memcpy(&body[0], data, length);
  //         len = length;
  //       }
  //       length -= offset;
  //       if (length > 0)
  //       {
  //         if (readFixSize(&body[len], length) <= 0)
  //         {
  //           close();
  //           return nullptr;
  //         }
  //       }
  //     }
  //   }
  //   std::cout << "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]" << std::endl;

  parser->getData()->setBody(body);
  r = parser->getData();
  return parser->getData();
}

int HttpConnection::sendRequest(HttpRequest::ptr request)
{
  std::stringstream ss;
  ss << *request;
  std::string data = ss.str();
  writeFixSize(data.c_str(), data.size());
  return data.size();
}

HttpResult::ptr HttpConnection::DoGet(const std::string& url, uint64_t timeout,
                                      const std::map<std::string, std::string>& headers, const std::string& body)
{

  URI::ptr uri = URI::Create(url);
  if (!uri)
    return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "Invalid uri: " + url);

  return DoGet(uri, timeout, headers, body);
}

HttpResult::ptr HttpConnection::DoGet(qslary::URI::ptr url, uint64_t timeout,
                                      const std::map<std::string, std::string>& headers, const std::string& body)
{
  return DoRequest(HttpMethod::GET, url, timeout, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(const std::string& url, uint64_t timeout,
                                       const std::map<std::string, std::string>& headers, const std::string& body)
{

  URI::ptr uri = URI::Create(url);
  if (!uri)
    return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "Invalid uri: " + url);

  return DoPost(uri, timeout, headers, body);
}

HttpResult::ptr HttpConnection::DoPost(qslary::URI::ptr url, uint64_t timeout,
                                       const std::map<std::string, std::string>& headers, const std::string& body)
{

  return DoRequest(HttpMethod::POST, url, timeout, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, const std::string& url, uint64_t timeout,
                                          const std::map<std::string, std::string>& headers, const std::string& body)
{
  URI::ptr uri = URI::Create(url);
  if (!uri)
    return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_URL, nullptr, "Invalid uri: " + url);

  return DoRequest(method, uri, timeout, headers, body);
}

HttpResult::ptr HttpConnection::DoRequest(HttpMethod method, qslary::URI::ptr uri, uint64_t timeout,
                                          const std::map<std::string, std::string>& headers, const std::string& body)
{

  HttpRequest::ptr req = std::make_shared<HttpRequest>();

  req->setPath(uri->getPath());
  req->setMethod(method);
  req->setQuery(uri->getQuery());
  req->setFragment(uri->getFragment());

  bool has_host = false;
  for (auto& i : headers)
  {
    if (strcasecmp(i.first.c_str(), "connection") == 0)
    {
      if (strcasecmp(i.second.c_str(), "keep-alive") == 0)
      {
        req->setClose(false);
      }
      continue;
    }
    if (!has_host && strcasecmp(i.first.c_str(), "host") == 0)
    {
      has_host = !i.second.empty();
    }
    req->setHeaderEntry(i.first, i.second);
  }
  if (has_host == false)
  {
    req->setHeaderEntry("Host", uri->getHost());
  }
  req->setBody(body);
  return DoRequest(req, uri, timeout);
}

HttpResult::ptr HttpConnection::DoRequest(HttpRequest::ptr req, qslary::URI::ptr uri, uint64_t timeout)
{
  IPAddress::ptr addr = uri->createAddress();
  if (!addr)
  {
    return std::make_shared<HttpResult>((int)HttpResult::Error::INVALID_HOST, nullptr,
                                        "Invalid host " + uri->getHost());
  }
  Socket::ptr sock = Socket::CreateTCPSocket(addr);
  if (!sock->connect(addr))
  {
    return std::make_shared<HttpResult>((int)HttpResult::Error::CONNECTION_FALID, nullptr,
                                        "Connection falid " + addr->toString());
  }

  sock->setRecvTimeout(timeout);
  std::cout << "iiiiiiiiiiiiiiiiiiiiii" << std::endl;

  HttpConnection::ptr conn = std::make_shared<HttpConnection>(sock);

  int rt = conn->sendRequest(req);
  if (rt == 0)
  {
    return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr,
                                        "send request closed by peer: " + addr->toString());
  }
  if (rt < 0)
  {
    return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr,
                                        "send request socket error errno=" + std::to_string(errno) +
                                            " errstr=" + std::string(strerror(errno)));
  }
  HttpResponse::ptr rsp;
  conn->recvResponse(rsp);
  if (!rsp)
  {
    return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr,
                                        "recv response timeout: " + addr->toString() +
                                            " timeout_ms:" + std::to_string(timeout));
  }
  return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "OK");
}

HttpConnectionPool::HttpConnectionPool(const std::string& host, const std::string& vhost, uint32_t port, 
                                       bool is_https, uint32_t max_size, uint32_t max_alive_time, uint32_t max_request)
    : host_(host), vhost_(vhost), port_(port ? port : (is_https ? 443 : 80)), maxSize_(max_size),
      maxAliveTime_(max_alive_time), maxRequest_(max_request)
{
}

HttpConnection::ptr HttpConnectionPool::getConnection()
{
  uint64_t now_ms = qslary::detail::GetCurrentMs();
  std::vector<HttpConnection*> invalid_conns;
  HttpConnection* ptr = nullptr;
  qslary::MutexLockGuard lock(mutex_);
  while (!conns_.empty())
  {
    auto conn = *conns_.begin();
    conns_.pop_front();
    if (!conn->isConnected())
    {
      invalid_conns.push_back(conn);
      continue;
    }
    if ((conn->create_time + maxAliveTime_) > now_ms)
    {
      invalid_conns.push_back(conn);
      continue;
    }
    ptr = conn;
    break;
  }
  for (auto i : invalid_conns)
  {
    delete i;
  }
  total_ -= invalid_conns.size();

  if (!ptr)
  {
    auto addrs = IPAddress::HostNameToAddress(host_);
    auto addr=addrs[0];
    if (!addr)
    {
      return nullptr;
    }
    addr->SetPort(port_);
    Socket::ptr sock = Socket::CreateTCPSocket(addr);
    if (!sock)
    {
      return nullptr;
    }
    if (!sock->connect(addr))
    {
      return nullptr;
    }

    ptr = new HttpConnection(sock);
    ++total_;
  }
  return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::ReleasePtr, std::placeholders::_1, this));
}

void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* pool)
{
  // ++ptr->m_request;
  if (!ptr->isConnected() || ((ptr->create_time + pool->maxAliveTime_) >= qslary::detail::GetCurrentMs()))
  {
    delete ptr;
    --pool->total_;
    return;
  }
  qslary::MutexLockGuard lock(pool->mutex_);
  pool->conns_.push_back(ptr);
}

HttpResult::ptr HttpConnectionPool::doPost(const std::string& url, uint64_t timeout_ms,
                                           const std::map<std::string, std::string>& headers, const std::string& body)
{
  return doRequest(HttpMethod::POST, url, timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doPost(URI::ptr uri, uint64_t timeout_ms,
                                           const std::map<std::string, std::string>& headers, const std::string& body)
{
  std::stringstream ss;
  ss << uri->getPath() << (uri->getQuery().empty() ? "" : "?") << uri->getQuery()
     << (uri->getFragment().empty() ? "" : "#") << uri->getFragment();
  return doPost(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doGet(URI::ptr uri, uint64_t timeout_ms,
                                          const std::map<std::string, std::string>& headers, const std::string& body)
{
  std::stringstream ss;
  ss << uri->getPath() << (uri->getQuery().empty() ? "" : "?") << uri->getQuery()
     << (uri->getFragment().empty() ? "" : "#") << uri->getFragment();
  return doGet(ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doGet(const std::string& url, uint64_t timeout_ms,
                                          const std::map<std::string, std::string>& headers, const std::string& body)
{
  return doRequest(HttpMethod::GET, url, timeout_ms, headers, body);
}


HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, const std::string& url, uint64_t timeout_ms,
                                              const std::map<std::string, std::string>& headers,
                                              const std::string& body)
{
  HttpRequest::ptr req = std::make_shared<HttpRequest>();
  req->setPath(url);
  req->setMethod(method);
  req->setClose(false);
  bool has_host = false;
  std::cout << "-----------" << headers.size() << std::endl;
  for (auto& i : headers)
  {
    if (strcasecmp(i.first.c_str(), "connection") == 0)
    {
      if (strcasecmp(i.second.c_str(), "keep-alive") == 0)
      {
        req->setClose(false);
      }
      continue;
    }

    if (!has_host && strcasecmp(i.first.c_str(), "host") == 0)
    {
      has_host = !i.second.empty();
    }

    req->setHeaderEntry(i.first, i.second);
  }
  if (!has_host)
  {
    if (vhost_.empty())
    {
      req->setHeaderEntry("Host", host_);
    }
    else
    {
      req->setHeaderEntry("Host", vhost_);
    }
  }
  req->setBody(body);
  return doRequest(req, timeout_ms);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, URI::ptr uri, uint64_t timeout_ms,
                                              const std::map<std::string, std::string>& headers,
                                              const std::string& body)
{
  std::stringstream ss;
  ss << uri->getPath() << (uri->getQuery().empty() ? "" : "?") << uri->getQuery()
     << (uri->getFragment().empty() ? "" : "#") << uri->getFragment();
  return doRequest(method, ss.str(), timeout_ms, headers, body);
}

HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req, uint64_t timeout_ms)
{
  auto conn = getConnection();
  if (!conn)
  {
    assert(false);
  }
  auto sock = conn->getSock();
  if (!sock)
  {
    assert(false);
  }

  sock->setRecvTimeout(timeout_ms);

  std::cout << "000000000000" << std::endl;
  std::cout << req->toString();
  std::cout << "000000000000" << std::endl;

  int rt = conn->sendRequest(req);

  if (rt == 0)
  {
    return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr,
                                        "send request closed by peer: " + sock->getRemoteAddress()->toString());
  }
  if (rt < 0)
  {
    return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_SOCKET_ERROR, nullptr,
                                        "send request socket error errno=" + std::to_string(errno) +
                                            " errstr=" + std::string(strerror(errno)));
  }
  HttpResponse::ptr xx;
  auto rsp = conn->recvResponse(xx);
  if (!rsp)
  {
    return std::make_shared<HttpResult>((int)HttpResult::Error::TIMEOUT, nullptr,
                                        "recv response timeout: " + sock->getRemoteAddress()->toString() +
                                            " timeout_ms:" + std::to_string(timeout_ms));
  }
  return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "ok");
}

// HttpConnectionPool::HttpConnectionPool(const std::string& host, const std::string& vhost, uint32_t port, uint32_t maxSize,
//                                        uint32_t maxAliveTime, uint32_t maxRequest)
//     : host_(host), vhost_(vhost), port_(port), maxSize_(maxSize), maxAliveTime_(maxAliveTime), maxRequest_(maxRequest)
// {
// }

// HttpConnection::ptr HttpConnectionPool::getConnection()
// {

//   uint64_t nowMs = qslary::detail::GetCurrentMs();
//   std::vector<HttpConnection*> inValid;
//   HttpConnection* ptr = nullptr;
//   {
//     qslary::MutexLockGuard lock(mutex_);
//     while (!conns_.empty())
//     {
//       auto conn = *conns_.begin();
//       conns_.pop_front();
//       if (!conn->isConnected())
//       {
//         inValid.push_back(conn);
//         continue;
//       }
//       if ((conn->create_time + maxAliveTime_) > nowMs)
//       {
//         inValid.push_back(conn);
//         continue;
//       }
//       ptr = conn;
//       break;
//     }
//     total_ -= inValid.size();
//   }

//   for (auto i : inValid)
//   {
//     delete i;
//   }
//   if (!ptr)
//   {
//     std::cout << "host" << host_ << std::endl;

//     auto addrs = IPAddress::HostNameToAddress(host_);
//     if (addrs.empty())
//     {
//       std::cout << "get Addr fail " << host_ << std::endl;
//     }
//     auto addr = addrs[0];
//     addr->SetPort(port_);
//     Socket::ptr sock = Socket::CreateTCPSocket(addr);
//     if (!sock)
//     {
//       std::cout << "get sock error " << host_ << std::endl;
//     }
//     if (!sock->connect(addr))
//     {
//       std::cout << "connection faild " << host_ << std::endl;
//     }
//     ++total_;
//   }
//   return HttpConnection::ptr(ptr, std::bind(&HttpConnectionPool::ReleasePtr,
//   std::placeholders::_1, this));
// }

// HttpResult::ptr HttpConnectionPool::doGet(const std::string& url, uint64_t timeout,
//                                           const std::map<std::string, std::string>& headers, const std::string& body)
// {
//   return doRequest(HttpMethod::GET, url, timeout, headers, body);
// }

// HttpResult::ptr HttpConnectionPool::doGet(qslary::URI::ptr uri, uint64_t timeout,
//                                           const std::map<std::string, std::string>& headers, const std::string& body)
// {
//   std::stringstream ss;
//   ss << uri->getPath() << (uri->getQuery().empty() ? "" : "?") << uri->getQuery()
//      << (uri->getFragment().empty() ? "" : "#") << uri->getFragment();

//   return doGet(ss.str(), timeout, headers, body);
// }

// HttpResult::ptr HttpConnectionPool::doPost(const std::string& url, uint64_t timeout,
//                                            const std::map<std::string, std::string>& headers, const std::string& body)
// {
//   return doRequest(HttpMethod::POST, url, timeout, headers, body);
// }

// HttpResult::ptr HttpConnectionPool::doPost(qslary::URI::ptr uri, uint64_t timeout,
//                                            const std::map<std::string, std::string>& headers, const std::string& body)
// {

//   std::stringstream ss;
//   ss << uri->getPath() << (uri->getQuery().empty() ? "" : "#") << uri->getQuery()
//      << (uri->getFragment().empty() ? "" : "#") << uri->getFragment();

//   return doPost(ss.str(), timeout, headers, body);
// }

// HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, const std::string& uri, uint64_t timeout,
//                                               const std::map<std::string, std::string>& headers,
//                                               const std::string& body)
// {

//   HttpRequest::ptr req = std::make_shared<HttpRequest>();

//   req->setPath(uri);
//   req->setMethod(method);
//   req->setClose(false);
//   bool has_host = false;
//   for (auto& i : headers)
//   {
//     if (strcasecmp(i.first.c_str(), "connection") == 0)
//     {
//       if (strcasecmp(i.second.c_str(), "keep-alive") == 0)
//       {
//         req->setClose(false);
//       }
//       continue;
//     }
//     if (!has_host && strcasecmp(i.first.c_str(), "host") == 0)
//     {
//       has_host = !i.second.empty();
//     }
//     req->setHeaderEntry(i.first, i.second);
//   }
//   if (has_host == false)
//   {
//     if (vhost_.empty())
//     {
//       req->setHeaderEntry("Host", host_);
//     }
//     else
//     {
//       req->setHeaderEntry("Host", vhost_);
//     }
//   }
//   req->setBody(body);

//   std::cout << req->toString() << std::endl;

//   return doRequest(req, timeout);
// }

// HttpResult::ptr HttpConnectionPool::doRequest(HttpMethod method, qslary::URI::ptr uri, uint64_t timeout,
//                                               const std::map<std::string, std::string>& headers,
//                                               const std::string& body)
// {

//   std::stringstream ss;
//   ss << uri->getPath() << (uri->getQuery().empty() ? "" : "#") << uri->getQuery()
//      << (uri->getFragment().empty() ? "" : "#") << uri->getFragment();

//   return doRequest(method, ss.str(), timeout, headers, body);
// }

// HttpResult::ptr HttpConnectionPool::doRequest(HttpRequest::ptr req, uint64_t timeout)
// {

//   auto conn = getConnection();
//   if (!conn)
//   {
//     return std::make_shared<HttpResult>((int)HttpResult::Error::POLL_GET_CONNECTION, nullptr,
//                                         "Poll Host:" + host_ + "Port: " + std::to_string(port_));
//   }

//   auto sock = conn->getSock();

//   if (!sock)
//   {
//     assert(false);
//   }

//   sock->setRecvTimeout(timeout);
//   int ret = conn->sendRequest(req);
//   if (ret == 0)
//   {
//     return std::make_shared<HttpResult>((int)HttpResult::Error::SEND_CLOSE_BY_PEER, nullptr,
//                                         "Send Request closed by peer: ");
//   }

//   if (ret < 0)
//   {
//     assert(false);
//   }

//   HttpResponse::ptr rsp;
//   conn->recvResponse(rsp);

//   if (!rsp)
//     assert(false);
//   return std::make_shared<HttpResult>((int)HttpResult::Error::OK, rsp, "OK");
// }

// void HttpConnectionPool::ReleasePtr(HttpConnection* ptr, HttpConnectionPool* poll)
// {

//   if (!ptr->isConnected()
//             || ((ptr->create_time + poll->maxAliveTime_) >= qslary::detail::GetCurrentMs()))
//   {
//     delete ptr;
//     --poll->maxSize_;
//     return;
//   }

//   poll->conns_.push_back(ptr);

// }

} // namespace http

} // namespace qslary

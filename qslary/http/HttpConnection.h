#ifndef __QSLARY_HTTP_CONNECTION_H_
#define __QSLARY_HTTP_CONNECTION_H_

#include "qslary/net/SockStream.h"
#include "qslary/http/http.h"
#include "qslary/net/URI.h"
#include "qslary/base/Mutex.h"
#include <atomic>
#include <cstdint>
#include <list>
#include <memory>
namespace qslary
{
namespace http
{

struct HttpResult
{
  typedef std::shared_ptr<HttpResult> ptr;

  enum class Error
  {
    OK = 0,
    INVALID_URL = 1,
    INVALID_HOST = 2,
    CONNECTION_FALID = 3,
    SEND_CLOSE_BY_PEER = 4,
    SEND_SOCKET_ERROR = 5,
    TIMEOUT = 6,
    // 从连接池中取连接失败
    POLL_GET_CONNECTION=7,
  };
  HttpResult(int result_, HttpResponse::ptr rsp, const std::string& err) : result(result_), response(rsp), error(err){};
  int result;
  HttpResponse::ptr response;
  std::string error;

  std::string toString() const;

};

class HttpConnectionPool;

class HttpConnection : public SockStream
{
  friend class HttpConnectionPool;
public:
  typedef std::shared_ptr<HttpConnection> ptr;

  HttpConnection(Socket::ptr sock, bool ower=true);

  static HttpResult::ptr DoGet(const std::string& url, uint64_t timeout,
                               const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

  static HttpResult::ptr DoGet(qslary::URI::ptr url, uint64_t timeout,
                               const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

  static HttpResult::ptr DoPost(const std::string& url, uint64_t timeout,
                               const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

  static HttpResult::ptr DoPost(qslary::URI::ptr url, uint64_t timeout,
                               const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

  static HttpResult::ptr DoRequest(HttpMethod method, const std::string& url, uint64_t timeout,
                                   const std::map<std::string, std::string>& headers = {},
                                   const std::string& body = "");

  static HttpResult::ptr DoRequest(HttpMethod method, qslary::URI::ptr uri, uint64_t timeout,
                                   const std::map<std::string, std::string>& headers = {},
                                   const std::string& body = "");

  static HttpResult::ptr DoRequest(HttpRequest::ptr req, qslary::URI::ptr uri, uint64_t timeout);
  

  HttpResponse::ptr recvResponse(HttpResponse::ptr& r);

  int sendRequest(HttpRequest::ptr request);

private:
  uint64_t create_time = 0;
};


class HttpConnectionPool
{
public:
  typedef std::shared_ptr<HttpConnectionPool> ptr;

  HttpConnectionPool(const std::string& host, const std::string& vhost, uint32_t port,bool is_https, uint32_t maxSize, uint32_t maxAliveTime,
                     uint32_t maxRequest);

  HttpConnection::ptr getConnection();

  HttpResult::ptr doGet(const std::string& url, uint64_t timeout,
                               const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

  HttpResult::ptr doGet(qslary::URI::ptr url, uint64_t timeout,
                               const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

  HttpResult::ptr doPost(const std::string& url, uint64_t timeout,
                                const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

  HttpResult::ptr doPost(qslary::URI::ptr url, uint64_t timeout,
                                const std::map<std::string, std::string>& headers = {}, const std::string& body = "");

  HttpResult::ptr doRequest(HttpMethod method, const std::string& url, uint64_t timeout,
                                   const std::map<std::string, std::string>& headers = {},
                                   const std::string& body = "");

  HttpResult::ptr doRequest(HttpMethod method, qslary::URI::ptr uri, uint64_t timeout,
                                   const std::map<std::string, std::string>& headers = {},
                                   const std::string& body = "");

  HttpResult::ptr doRequest(HttpRequest::ptr req,  uint64_t timeout);


private:
  static void ReleasePtr(HttpConnection* ptr, HttpConnectionPool* poll);

  struct ConnectionInfo
  {
    HttpConnection* conn;
    uint64_t create_time;
  };
  
private : std::string host_;
  std::string vhost_;
  uint32_t port_;
  uint32_t maxSize_;
  uint32_t maxAliveTime_;
  uint32_t maxRequest_;

  qslary::MutexLock mutex_;
  std::list<HttpConnection*> conns_;
  std::atomic<int32_t> total_ = {0};

};


} // namespace http
} // namespace qslary

#endif
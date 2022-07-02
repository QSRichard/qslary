#ifndef __QSLARY_HTTP_SESSION_H_
#define __QSLARY_HTTP_SESSION_H_

#include "qslary/net/SockStream.h"
#include "qslary/http/http.h"

#include <memory>
namespace qslary
{
namespace http
{
class HttpSession : public SockStream
{
public:
  typedef std::shared_ptr<HttpSession> ptr;
  HttpSession(Socket::ptr sock, bool ower);

  HttpRequest::ptr recvRequest(HttpRequest::ptr& r);

  int sendResponse(HttpResponse::ptr response);

};
}
}

#endif
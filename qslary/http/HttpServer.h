#ifndef __QSLARY_HTTP_SERVER_H_
#define __QSLARY_HTTP_SERVER_H_

#include "qslary/http/HttpSession.h"
#include "qslary/http/Servlet.h"
#include "qslary/net/IOManager.h"
#include "qslary/net/TcpServer.h"
#include <memory>

namespace qslary
{
namespace http
{
class HttpServer : public TcpServer
{
public:
    typedef std::shared_ptr<HttpServer> ptr;
    HttpServer(
        bool keepAlive = false,
        qslary::IOManager *work = qslary::IOManager::GetIOManager(),
        qslary::IOManager *accept_worker = qslary::IOManager::GetIOManager());

    ServletDispatch::ptr getServletDispatch() { return dispatch_; }

    void setServletDispatch(ServletDispatch::ptr slt) { dispatch_ = slt; }

protected:
    void handleClient(Socket::ptr client) override;

private:
    bool keepAlive_;
    ServletDispatch::ptr dispatch_;
};
} // namespace http
} // namespace qslary

#endif
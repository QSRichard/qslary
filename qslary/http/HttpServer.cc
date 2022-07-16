#include "qslary/http/HttpServer.h"
#include "qslary/net/IOManager.h"

namespace qslary
{
namespace http
{
HttpServer::HttpServer(bool keepAlive, qslary::IOManager *work,
                       IOManager *accept_worker)
    : TcpServer(work, accept_worker), keepAlive_(keepAlive)
{
    dispatch_.reset(new ServletDispatch);
}

void HttpServer::handleClient(Socket::ptr client)
{
    static int number = 0;
    HttpSession::ptr session(new HttpSession(client, false));
    do
    {
        number++;
        std::cout << std::endl
                  << "number = " << number << std::endl
                  << std::endl;
        HttpRequest::ptr t;
        auto request = session->recvRequest(t);
        std::cout << request << std::endl;
        if (!request)
        {
            // TODO  add logging
            assert(false);
        }

        HttpResponse::ptr response(
            new HttpResponse(HttpStatus::OK, request->getVersion(),
                             request->isClose() || !keepAlive_));

        dispatch_->handle(request, response, session);
        // response->setBody("hello qslary");
        session->sendResponse(response);
        if (!keepAlive_ || response->getClose())
        {
            std::cout << "从break中跳了出来" << std::endl;
            break;
        }
    }
    while (true);

    session->close();
}
} // namespace http
} // namespace qslary
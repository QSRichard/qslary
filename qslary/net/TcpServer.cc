#include "qslary/base/Config.h"
#include "qslary/base/CurrentThread.h"
#include "qslary/base/Thread.h"
#include "qslary/net/IOManager.h"
#include "qslary/net/TcpServer.h"
#include <cstdint>

namespace qslary
{
static qslary::ConfigVar<uint64_t>::ptr g_tcp_server_readTimeout =
    qslary::Config::lookup("tcp_server.read_timeout", (uint64_t)(60 * 1000 * 2),
                           "tcp server read timeout");

TcpServer::TcpServer(IOManager *accept_worker, IOManager *worker)
    : accept_worker_(accept_worker), worker_(worker),
      recvTimeout_(g_tcp_server_readTimeout->getValue()), name_("qslary/1.0.0"),
      isStop_(true)
{}

TcpServer::~TcpServer()
{
    for (auto &i : sockets_)
    {
        i->close();
    }
}

bool TcpServer::bind(Address::ptr addr)
{
    std::vector<Address::ptr> addrs, fails;
    addrs.push_back(addr);
    return bind(addrs, fails);
}

bool TcpServer::bind(const std::vector<Address::ptr> &addrs,
                     std::vector<Address::ptr> &fails)
{
    for (auto &i : addrs)
    {
        Socket::ptr sock = Socket::CreateTCPSocket(i);
        if (!sock->bind(i))
        {
            // TODO logging
            fails.push_back(i);
        }
        if (!sock->listen())
        {
            // TODO logging
            fails.push_back(i);
            continue;
        }
        sockets_.push_back(sock);
    }
    for (auto &i : sockets_)
    {
        std::cout << "Server bind success: " << *i;
    }
    return fails.empty();
}

bool TcpServer::start()
{
    if (isStop_ == false)
        return true;
    isStop_ = false;

    for (auto &sock : sockets_)
    {
        accept_worker_->scheduler(
            std::bind(&TcpServer::startAccept, shared_from_this(), sock));
    }
    return true;
}

void TcpServer::startAccept(Socket::ptr sock)
{
    while (isStop_ == false)
    {
        Socket::ptr client = sock->accept();
        std::cout << "TcpServer::startAccept Sock->accept返回" << std::endl;
        if (client)
        {
            std::cout << "client " << *client << std::endl;
            client->setRecvTimeout(recvTimeout_);
            worker_->scheduler(std::bind(&TcpServer::handleClient,
                                         shared_from_this(), client));
        }
        else
            // TODO logging
            ;
    }
}

bool TcpServer::stop()
{
    isStop_ = true;
    auto self = shared_from_this();
    accept_worker_->scheduler([this, self]() {
        for (auto &sock : sockets_)
        {
            sock->cancelAll();
            sock->close();
        }
        sockets_.clear();
    });
    return true;
}

void TcpServer::handleClient(Socket::ptr client)
{
    std::cout << " ==============================TcpServer handleClient"
              << std::endl;
}

} // namespace qslary
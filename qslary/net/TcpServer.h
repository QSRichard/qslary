#ifndef __QSLARY_TCP_SERVER_H_
#define __QSLARY_TCP_SERVER_H_

#include "qslary/base/noncopyable.h"
#include "qslary/net/Address.h"
#include "qslary/net/IOManager.h"
#include "qslary/net/Socket.h"

#include <cstdint>
#include <memory>
#include <vector>
namespace qslary
{
class TcpServer : public std::enable_shared_from_this<TcpServer>, noncopyable
{
public:
    typedef std::shared_ptr<TcpServer> ptr;

    TcpServer(IOManager *accept_worker = IOManager::GetIOManager(),
              IOManager *worker = IOManager::GetIOManager());
    virtual ~TcpServer();

    virtual bool bind(Address::ptr addr);
    virtual bool bind(const std::vector<Address::ptr> &addrs,
                      std::vector<Address::ptr> &fails);
    virtual bool start();
    virtual bool stop();

    uint64_t getReadTimeout() const { return recvTimeout_; }

protected:
    virtual void handleClient(Socket::ptr client);
    virtual void startAccept(Socket::ptr sock);

private:
    // TcpServer 自身监听socket
    std::vector<Socket::ptr> sockets_;
    IOManager *accept_worker_;
    IOManager *worker_;
    uint64_t recvTimeout_;
    std::string name_;
    bool isStop_;
};
} // namespace qslary

#endif
#ifndef __QSLARY_SOCKET_H_
#define __QSLARY_SOCKET_H_

#include "qslary/net/Address.h"
#include "qslary/base/noncopyable.h"
#include <bits/types/struct_iovec.h>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <netinet/tcp.h>
#include <ostream>
#include <sys/socket.h>
#include <sys/types.h>
namespace qslary
{
class Socket : public std::enable_shared_from_this<Socket>,noncopyable
{
public:
  typedef std::shared_ptr<Socket> ptr;
  typedef std::weak_ptr<Socket> weak_ptr;

  static Socket::ptr CreateTCPSocket(qslary::Address::ptr addr);
  static Socket::ptr CreateUDPSocket(qslary::Address::ptr addr);

  static Socket::ptr CreateTCPSocket();
  static Socket::ptr CreateUDPSocket();
  static Socket::ptr CreateTCPSocket6();
  static Socket::ptr CreateUDPSocket6();

  static Socket::ptr CreateTCPUnixSocket(qslary::Address::ptr addr);
  static Socket::ptr CreateUDPUnixSocket(qslary::Address::ptr addr);

  Socket(int family, int type, int protocol = 0);
  ~Socket();


  bool getOption(int level, int option, void* result, size_t* len) const;
  template <class T> bool getOption(int level, int option, T& result)
  {
    size_t length = sizeof(T);
    return getOption(level,option,&result,length);
  }

  bool setOption(int level, int option, const void* result, size_t len);
  template <class T> bool setOption(int level, int option, const T& result)
  {
    return setOption(level,option,&result,sizeof(result));
  }

  Socket::ptr accept();
  // 将fd转为socket
  bool init(int fd);
  bool bind(const Address::ptr addr);
  bool connect(const Address::ptr addr, uint64_t timeout_ms = -1);
  bool listen(int backlog = SOMAXCONN);
  bool close();

  int send(const void* buffer, size_t length, int flag = 0);
  int send(const iovec* buffers, size_t length, int flags = 0);
  int sendTo(const void* buffer, size_t length, const Address::ptr to, int flag = 0);
  int sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flag = 0);

  int recv(void* buffer, size_t length, int flag = 0);
  int recv(iovec* buffers, size_t length, int flag = 0);
  int recvFrom(void* buffers, size_t length, Address::ptr from, int flag = 0);
  int recvFrom(iovec* buffers, size_t length, Address::ptr from, int flag = 0);

  Address::ptr getRemoteAddress();
  Address::ptr getLocalAddress();

  bool setSendTimeout(uint64_t val);
  bool setRecvTimeout(uint64_t val);

  uint64_t getSendTimeout() const;
  uint64_t getRecvTimeout() const;
  int getFamily() const
  {
    return family_;
  }


  int getType() const
  {
    return type_;
  }

  int getProtocol() const
  {
    return protocol_;
  }

  int getSocket() const
  {
    return socketfd_;
  }
  bool isConnected() const
  {
    return isConnected_;
  }
  bool isVaild() const;
  bool getError() const;

  std::ostream& dump(std::ostream& os) const;

  bool cancelRead();
  bool cancelWrite();
  bool cancelAccept();
  bool cancelAll();

private:
  void initSock();
  void newSock();

private:
  int socketfd_;
  int family_;
  int type_;
  int protocol_;
  bool isConnected_;

  Address::ptr localAddress_;
  Address::ptr remoteAddress_;
};

std::ostream& operator<<(std::ostream& os, const Socket& addr);

} // namespce qslary


#endif // __QSLARY_SOCKET_H

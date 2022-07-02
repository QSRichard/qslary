#include "qslary/net/Socket.h"
#include "qslary/net/FdManager.h"
#include "qslary/net/IOManager.h"
#include "qslary/base/Logger.h"
#include "qslary/base/Macro.h"

#include <bits/types/struct_iovec.h>
#include <bits/types/struct_timeval.h>
#include <cstdint>
#include <cstring>
#include <memory>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>

namespace qslary
{

static qslary::Logger::ptr g_logger = QSLARY_LOG_NAME("system");


Socket::ptr Socket::CreateTCPSocket(qslary::Address::ptr addr)
{
  Socket::ptr sock(new Socket(addr->getFamily(), SOCK_STREAM, 0));
  return sock;
}
Socket::ptr Socket::CreateUDPSocket(qslary::Address::ptr addr)
{
  Socket::ptr sock(new Socket(addr->getFamily(), SOCK_DGRAM, 0));
  return sock;
}

Socket::ptr Socket::CreateTCPSocket()
{
  Socket::ptr sock(new Socket(AF_INET, SOCK_STREAM, 0));
  return sock;
}
Socket::ptr Socket::CreateUDPSocket()
{
  Socket::ptr sock(new Socket(AF_INET, SOCK_DGRAM, 0));
  return sock;
}

Socket::ptr Socket::CreateTCPSocket6()
{
  Socket::ptr sock(new Socket(AF_INET6, SOCK_STREAM, 0));
  return sock;
}
Socket::ptr Socket::CreateUDPSocket6()
{
  Socket::ptr sock(new Socket(AF_INET6, SOCK_DGRAM, 0));
  return sock;
}

Socket::ptr Socket::CreateTCPUnixSocket(qslary::Address::ptr addr)
{
  Socket::ptr sock(new Socket(AF_UNIX, SOCK_STREAM, 0));
  return sock;
}
Socket::ptr Socket::CreateUDPUnixSocket(qslary::Address::ptr addr)
{
  Socket::ptr sock(new Socket(AF_UNIX, SOCK_DGRAM, 0));
  return sock;
}

Socket::Socket(int family, int type, int protocol)
    : socketfd_(-1), family_(family), type_(type), protocol_(protocol), isConnected_(false)
{
}
Socket::~Socket()
{
  close();
}

bool Socket::getOption(int level, int option, void* result, size_t* len) const
{
  int rt = getsockopt(socketfd_, level, option, result, (socklen_t*)len);
  assert(rt == 0);
  return rt==0;
}

bool  Socket::setOption(int level, int option, const void* result, size_t len)
{
  int rt = setsockopt(socketfd_, level, option, result, (socklen_t)len);
  assert(rt == 0);
  return rt==0;
}

// 向系统申请socket拿到fd
void Socket::newSock()
{
  socketfd_ = socket(family_, type_, protocol_);
  if (QSLARY_LICKLY(socketfd_ != -1))
    initSock();
  else
    QSLARY_LOG_ERROR(g_logger) << "socket( " << family_ << ", " << type_ << " ," << protocol_ << " ) errno " << errno
                               << " errstr" << strerror(errno);
}

// 设置Sock重用端口地址 禁用Nagle算法
void Socket::initSock()
{
  int val = 1;
  setOption(SOL_SOCKET, SO_REUSEADDR, val);
  if (type_ == SOCK_STREAM)
    // 启动TCP_NODELAY，就意味着禁用了Nagle算法
    setOption(IPPROTO_TCP, TCP_NODELAY, val);
}

// 传入fd并对sock做初始化
bool Socket::init(int fd)
{
  FdContext::ptr fd_ctx = FdMgr::getInstance()->getFdContext(fd);
  if (fd_ctx && fd_ctx->isSocket() && !fd_ctx->isClosed())
  {
    socketfd_ = fd;
    isConnected_ = true;
    initSock();
    getLocalAddress();
    getRemoteAddress();
    return true;
  }
  return false;
}

Socket::ptr Socket::accept()
{
  Socket::ptr sock(new Socket(family_, type_, protocol_));
  int newfd = ::accept(socketfd_, nullptr, nullptr);
  if (newfd == -1)
  {
    QSLARY_LOG_ERROR(g_logger) << "accept( " << socketfd_ << " ) errno = " << errno << " errstr " << strerror(errno);
    return nullptr;
  }
  if (sock->init(newfd))
    return sock;
  return nullptr;
}

bool Socket::bind(const Address::ptr addr)
{
  if (!isVaild())
    newSock();
  if (addr->getFamily() != family_)
  {
    QSLARY_LOG_ERROR(g_logger) << "bind family nonequall error";
    return false;
  }
  if (::bind(socketfd_, addr->getAddr(), addr->getAddrLen()))
  {
    QSLARY_LOG_ERROR(g_logger)<<"::bind error errno "<<errno<<" errnostr "<<strerror(errno);
    return false;
  }
  getLocalAddress();
  return true;
  
}
bool Socket::connect(const Address::ptr addr, uint64_t timeout_ms)
{
  if (!isVaild())
    newSock();
  if (QSLARY_UNLICKLY(!isVaild()))
  {
    QSLARY_LOG_ERROR(g_logger)<<"newSock errno";
    return false;
  }

  if (QSLARY_UNLICKLY(addr->getFamily() != family_))
  {
    QSLARY_LOG_ERROR(g_logger) << "bind family nonequall error";
    return false;
  }
  if (timeout_ms == (uint64_t)(-1))
  {
    if (::connect(socketfd_, addr->getAddr(), addr->getAddrLen()))
    {
      QSLARY_LOG_ERROR(g_logger) << "sock = " << socketfd_ << " connect error errno = " << errno << "error str "
                                 << strerror(errno);
      close();
      return false;
    }
  }
  else if (::connect(socketfd_, addr->getAddr(), addr->getAddrLen()))
  {
    QSLARY_LOG_ERROR(g_logger) << "sock = " << socketfd_ << " connect error errno = " << errno << "error str "
                               << strerror(errno);
    close();
    return false;
  }
  isConnected_ = true;
  getRemoteAddress();
  getLocalAddress();
  return true;
}


bool Socket::listen(int backlog)
{
  if (!isVaild())
  {
    QSLARY_LOG_ERROR(g_logger) << "listen error sock ";
    return false;
  }
  return ::listen(socketfd_, backlog)==0;
}


bool Socket::close()
{
  if (isConnected_ == false && socketfd_ == -1)
    return true;
  isConnected_ = false;
  if (socketfd_ != -1)
  {
    ::close(socketfd_);
    socketfd_=-1;
  }
  // FIXME 
  return true;
}

int Socket::send(const void* buffer, size_t length, int flags)
{
  if (isConnected())
    return ::send(socketfd_, buffer, length, flags);
  return -1;
}
int Socket::send(const iovec* buffers, size_t length, int flags)
{
  if (isConnected())
  {
    msghdr msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    return ::sendmsg(socketfd_,&msg,flags);
  }
  return -1;
}


int Socket::sendTo(const void* buffer, size_t length, const Address::ptr to, int flags)
{
  if (isConnected())
    return ::sendto(socketfd_, buffer, length, flags, to->getAddr(), to->getAddrLen());
  return -1;
}
int Socket::sendTo(const iovec* buffers, size_t length, const Address::ptr to, int flags)
{
  if (isConnected())
  {
    msghdr msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.msg_iov = (iovec*)buffers;
    msg.msg_iovlen = length;
    msg.msg_name = to->getAddr();
    msg.msg_namelen = to->getAddrLen();
    return ::sendmsg(socketfd_,&msg,flags);
  }
  return -1;
}

int Socket::recv(void* buffer, size_t length, int flags)
{
  if (isConnected())
    return ::recv(socketfd_, buffer, length, flags);
  return -1;
}
int Socket::recv(iovec* buffers, size_t length, int flags)
{
  if (isConnected())
  {
    msghdr msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.msg_iov=buffers;
    msg.msg_iovlen = length;
    return ::recvmsg(socketfd_, &msg, flags);
  }
  return -1;
}

int Socket::recvFrom(void* buffer, size_t length, Address::ptr from, int flags)
{
  if (isConnected())
  {
    // revcfrom 的最后一个参数需要一个socklen_t 的地址 因此需要在此定义一个变量
    socklen_t len = from->getAddrLen();
    return ::recvfrom(socketfd_, buffer, length, flags, const_cast<sockaddr*>(from->getAddr()),&len);
  }
  return -1;
}

int Socket::recvFrom(iovec* buffers, size_t length, Address::ptr from, int flags)
{
  if (isConnected())
  {
    msghdr msg;
    std::memset(&msg, 0, sizeof(msg));
    msg.msg_iovlen = length;
    msg.msg_iov = buffers;
    msg.msg_name = const_cast<sockaddr*>(from->getAddr());
    msg.msg_namelen = from->getAddrLen();
    return ::recvmsg(socketfd_, &msg, flags);
  }
  return -1;
}

Address::ptr Socket::getRemoteAddress()
{
  if (remoteAddress_)
    return remoteAddress_;
  Address::ptr result;
  switch (family_)
  {
  case AF_INET:
    result.reset(new IPv4Address);
    break;
  case AF_INET6:
    result.reset(new IPv6Address);
    break;
  case AF_UNIX:
    result.reset(new UnixAddress);
    break;
  default:
    assert(false);
  }

  socklen_t addrlen = result->getAddrLen();
  if (getpeername(socketfd_, const_cast<sockaddr*>(result->getAddr()), &addrlen))
  {
    QSLARY_LOG_ERROR(g_logger) << "getpeername error sock = " << socketfd_ << " errno " << errno << " str errno "
                               << strerror(errno);
    return nullptr;
  }
  if (family_ == AF_UNIX)
  {
    UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
    // FIXME dynamic_pointer_cast
    addr->setAddrlen(addrlen);
  }
  remoteAddress_ = result;
  return remoteAddress_;
}
Address::ptr Socket::getLocalAddress()
{
  if (localAddress_)
    return localAddress_;
  Address::ptr result;
  switch (family_)
  {
  case AF_INET:
    result.reset(new IPv4Address);
    break;
  case AF_INET6:
    result.reset(new IPv6Address);
    break;
  case AF_UNIX:
    result.reset(new UnixAddress);
    break;
  default:
    assert(false);
  }

  socklen_t addrlen = result->getAddrLen();
  if (getsockname(socketfd_, const_cast<sockaddr*>(result->getAddr()), &addrlen))
  {
    QSLARY_LOG_ERROR(g_logger) << "getsockname error sock = " << socketfd_ << " errno " << errno << " str errno "
                               << strerror(errno);
    return nullptr;
  }
  if (family_ == AF_UNIX)
  {
    // FIXME dynamic_pointer_cast
    UnixAddress::ptr addr = std::dynamic_pointer_cast<UnixAddress>(result);
    addr->setAddrlen(addrlen);
  }
  localAddress_ = result;
  return localAddress_;
}

bool Socket::setSendTimeout(uint64_t val)
{
  struct timeval tv
  {
    static_cast<int>(val / 1000),static_cast<int>((val%1000)*1000)
  };
  return setOption(SOL_SOCKET,SO_SNDTIMEO,tv);
}

bool Socket::setRecvTimeout(uint64_t val)
{
  struct timeval tv
  {
    static_cast<int>(val / 1000), static_cast<int>((val % 1000) * 1000)
  };
  return setOption(SOL_SOCKET,SO_RCVTIMEO,tv);
}

uint64_t Socket::getSendTimeout() const
{
  qslary::FdContext::ptr fd_ctx = qslary::FdMgr::getInstance()->getFdContext(socketfd_);
  assert(fd_ctx);
  return fd_ctx->getTimeout(SO_SNDTIMEO);
  
}
uint64_t Socket::getRecvTimeout() const
{
  qslary::FdContext::ptr fd_ctx = qslary::FdMgr::getInstance()->getFdContext(socketfd_);
  assert(fd_ctx);
  return fd_ctx->getTimeout(SO_RCVTIMEO);
}

bool Socket::isVaild() const
{
  return socketfd_!=-1;
}
bool Socket::getError() const
{
  int error = 0;
  size_t len = sizeof(error);
  if (getOption(SOL_SOCKET, SO_ERROR, &error,&len)==false)
    return -1;
  return error;
}

std::ostream& Socket::dump(std::ostream& os) const
{
  os << " Socket sock= " << socketfd_ << " is connect= " << isConnected_ << " family = " << family_ << " type " << type_
     << " protocol " << protocol_;
  if (localAddress_)
    os << " localAddress = " << localAddress_->toString();
  if (remoteAddress_)
    os << " remoteAddress = " << remoteAddress_->toString();
  os << std::endl;
  return os;
}

bool Socket::cancelRead()
{
  return IOManager::GetIOManager()->cancelEvent(socketfd_, qslary::IOManager::READ);
}
bool Socket::cancelWrite()
{
  return IOManager::GetIOManager()->cancelEvent(socketfd_, qslary::IOManager::WRITE);
}
bool Socket::cancelAccept()
{
  return IOManager::GetIOManager()->cancelEvent(socketfd_, qslary::IOManager::READ);
}
bool Socket::cancelAll() { return IOManager::GetIOManager()->cancelAll(socketfd_); }

std::ostream& operator<<(std::ostream& os, const Socket& addr)
{
  return addr.dump(os);
}

} // namespace qslary
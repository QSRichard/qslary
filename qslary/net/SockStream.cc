#include "qslary/net/SockStream.h"
#include <bits/types/struct_iovec.h>

namespace qslary
{


SockStream::SockStream(Socket::ptr sock, bool ower) : sock_(sock), ower_(ower) {}

SockStream::~SockStream()
{
  if (sock_ && ower_)
    sock_->close();
}

int SockStream::read(void* buffer, size_t length)
{
  if (isConnected() == false)
    return -1;
  return sock_->recv(buffer, length);
}


int SockStream::read(ByteArray::ptr bytearray, size_t length)
{
  if (isConnected() == false)
    return -1;
  std::vector<struct iovec> ioves;
  bytearray->getWriteBuffers(ioves, length);
  int ret = sock_->recv(&ioves[0], ioves.size());

  // 向ByteArray中写入了数据 需要改变ByteArray的状态
  if (ret > 0)
    bytearray->setPosition(bytearray->getPosition() + ret);
  return ret; 
}


int SockStream::write(const void* buffer, size_t length)
{

  if (isConnected() == false)
    return -1;
  return sock_->send(buffer, length);
  
}

int SockStream::write(ByteArray::ptr bytearray, size_t length)
{
  if (isConnected() == false)
    return -1;
  std::vector<struct iovec> ioves;
  bytearray->getReadBuffers(ioves, length);
  int ret = sock_->send(&ioves[0], ioves.size());
  if (ret > 0)
    bytearray->setPosition(bytearray->getPosition() + ret);
  return ret;
}

void SockStream::close()
{
  if (sock_)
    sock_->close();
}

inline bool SockStream::isConnected() const { return sock_ && sock_->isConnected(); }

}
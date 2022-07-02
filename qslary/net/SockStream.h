#ifndef __QSLARY_SOCK_STREAM_H_
#define __QSLARY_SOCK_STREAM_H_

#include "qslary/net/Socket.h"
#include "qslary/net/Stream.h"

#include <memory>

namespace qslary
{

class SockStream : public Stream
{
public:
  typedef std::shared_ptr<SockStream> ptr;
  SockStream(Socket::ptr sock, bool ower = true);
  ~SockStream();
  
  int read(void* buffer, size_t length) override;
  int read(ByteArray::ptr bytearray, size_t length) override;
  int write(const void* buffer, size_t length) override;
  int write(ByteArray::ptr bytearray, size_t length) override;
  void close() override;

  bool isConnected() const;

  Socket::ptr getSock() const { return sock_; }
  bool getOwer() const { return ower_; }
  
private:
  Socket::ptr sock_;
  bool ower_;

};

}


#endif
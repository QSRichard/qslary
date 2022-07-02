#ifndef __QSLARY_HOOK_H_
#define __QSLARY_HOOK_H_

#include <bits/types/struct_timespec.h>
#include <cstdint>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

namespace qslary
{

// extern 和 static 不能一起使用
extern thread_local bool thread_hook_enable;
inline bool is_hook_enable()
{
  return thread_hook_enable;
}
inline void set_hook_enable(bool flag)
{
  thread_hook_enable = flag;
}

} // namespace qslary

extern "C"
{

  /**
   * @brief sleep 相关函数
   *
   */
  typedef unsigned int (*sleep_fun)(unsigned int seconds);
  extern sleep_fun sleep_f;

  typedef int (*usleep_fun)(useconds_t usec);
  extern usleep_fun usleep_f;

  typedef int (*nanaosleep_fun)(const struct timespec* reg, struct timespec* rem);
  extern nanaosleep_fun nanaosleep_f;

  /**
   * @brief socket 相关函数
   *
   */
  typedef int (*socket_fun)(int domain, int type, int protocol);
  extern socket_fun socket_f;

  typedef int (*connect_fun)(int sockfd, const struct sockaddr* addr, socklen_t addrlen);
  extern connect_fun connect_f;

  extern int connect_with_timeout(int sockfd, const struct sockaddr* addr, socklen_t addrlen, uint64_t timeout_ms);

  typedef int (*accept_fun)(int s, struct sockaddr* addr, socklen_t* addrlen);
  extern accept_fun accept_f;

  typedef int (*close_fun)(int fd);
  extern close_fun close_f;

  /**
   * @brief read 相关
   *
   */
  typedef ssize_t (*read_fun)(int fd, void* buf, size_t count);
  extern read_fun read_f;

  typedef ssize_t (*readv_fun)(int fd, const struct iovec* iov, int iovcnt);
  extern readv_fun readv_f;

  typedef ssize_t (*recv_fun)(int sockfd, void* buf, size_t len, int flags);
  extern recv_fun recv_f;

  typedef ssize_t (*recvfrom_fun)(int sockfd, void* buf, size_t len, int flags, struct sockaddr* src_addr,
                                  socklen_t* addrlen);
  extern recvfrom_fun recvfrom_f;

  typedef ssize_t (*recvmsg_fun)(int sockfd, struct msghdr* msg, int flags);
  extern recvmsg_fun recvmsg_f;

  /**
   * @brief write 相关
   *
   */
  typedef ssize_t (*write_fun)(int fd, const void* buf, size_t count);
  extern write_fun write_f;

  typedef ssize_t (*writev_fun)(int fd, const struct iovec* iov, int iovcnt);
  extern writev_fun writev_f;

  typedef ssize_t (*send_fun)(int sockfd, const void* buf, size_t len, int flags);
  extern send_fun send_f;

  typedef ssize_t (*sendto_fun)(int sockfd, const void* buf, size_t len, int flags, const struct sockaddr* dest_addr,
                                socklen_t addrlen);
  extern sendto_fun sendto_f;

  typedef ssize_t (*sendmsg_fun)(int sockfd, const struct msghdr* msg, int flags);
  extern sendmsg_fun sendmsg_f;

  /**
   * @brief 设置相关
   *
   */
  typedef int (*fcntl_fun)(int fd, int cmd, ... /* arg */);
  extern fcntl_fun fcntl_f;

  typedef int (*ioctl_fun)(int fd, unsigned long request, ...);
  extern ioctl_fun ioctl_f;

  typedef int (*getsockopt_fun)(int sockfd, int level, int optname, void* optval, socklen_t* optlen);
  extern getsockopt_fun getsockopt_f;

  typedef int (*setsockopt_fun)(int sockfd, int level, int optname, const void* optval, socklen_t optlen);
  extern setsockopt_fun setsockopt_f;

} // extenr "C" end

#endif // __QSLARY_HOOK_H_
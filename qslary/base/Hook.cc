
#include "qslary/base/Config.h"
#include "qslary/base/Fiber.h"
#include "qslary/base/Hook.h"
#include "qslary/base/Logger.h"
#include "qslary/net/FdManager.h"
#include "qslary/net/IOManager.h"
#include "qslary/net/Timer.h"

#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <asm-generic/socket.h>
#include <bits/types/struct_iovec.h>
#include <bits/types/struct_timespec.h>
#include <bits/types/struct_timeval.h>
#include <cstddef>
#include <cstdint>
#include <dlfcn.h>
#include <functional>
#include <memory>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>

namespace qslary {

static Logger::ptr g_logger = QSLARY_LOG_NAME("system");

static qslary::ConfigVar<int>::ptr g_tcp_connect_timeout =
    qslary::Config::lookup("tcp.connect.timeout", 50000, "tcp connect timeout");

thread_local bool thread_hook_enable = false;

#define HOOK_FUNCTION(XX)                                                      \
  XX(sleep)                                                                    \
  XX(usleep)                                                                   \
  XX(nanaosleep)                                                               \
  XX(socket)                                                                   \
  XX(connect)                                                                  \
  XX(accept)                                                                   \
  XX(read)                                                                     \
  XX(readv)                                                                    \
  XX(recv)                                                                     \
  XX(recvfrom)                                                                 \
  XX(recvmsg)                                                                  \
  XX(write)                                                                    \
  XX(writev)                                                                   \
  XX(send)                                                                     \
  XX(sendto)                                                                   \
  XX(sendmsg)                                                                  \
  XX(close)                                                                    \
  XX(fcntl)                                                                    \
  XX(ioctl)                                                                    \
  XX(getsockopt)                                                               \
  XX(setsockopt)

void hook_init() {
  static bool is_inited = false;
  if (is_inited) {
    return;
  }
#define XX(name) name##_f = (name##_fun)dlsym(RTLD_NEXT, #name);
  HOOK_FUNCTION(XX);
#undef XX
}

static uint64_t s_connect_timeout = -1;
struct _HookIniter {
  _HookIniter() {
    hook_init();
    s_connect_timeout = g_tcp_connect_timeout->getValue();
    // QSLARY_LOG_DEBUG(g_logger)<<"s_connect_timeout "<<s_connect_timeout;
    g_tcp_connect_timeout->addListener(
        [](const int &old_value, const int &new_value) {
          QSLARY_LOG_INFO(g_logger) << "tcp connect timeout update"
                                    << "new value " << new_value;
          s_connect_timeout = new_value;
        });
  }
};
// 在此处调用 hook_init()
static _HookIniter static_hook_initer;

struct timer_info {
  int cancelled = 0;
};

template <typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char *hook_fun_name,
                     uint32_t event, int timeout_so, Args &&...args) {
  if (qslary::thread_hook_enable == false)
    return fun(fd, std::forward<Args>(args)...);

  // QSLARY_LOG_DEBUG(g_logger)<<hook_fun_name<<" do io...";
  qslary::FdContext::ptr fdctx = qslary::FdMgr::getInstance()->getFdContext(fd);
  if (!fdctx)
    return fun(fd, std::forward<Args>(args)...);
  if (fdctx->isClosed()) {
    errno = EBADF;
    return -1;
  }
  if (!fdctx->isSocket() || fdctx->getUserNonBlock())
    return fun(fd, std::forward<Args>(args)...);

  uint64_t to = fdctx->getTimeout(timeout_so);
  std::shared_ptr<timer_info> tinfo(new timer_info);

retry:
  ssize_t n = fun(fd, std::forward<Args>(args)...);
  while (n == -1 && errno == EINTR) {
    n = fun(fd, std::forward<Args>(args)...);
  }
  if (n == -1 && errno == EAGAIN) {
    QSLARY_LOG_DEBUG(g_logger) << "do io EAGIN...";
    qslary::IOManager *iomanager = qslary::IOManager::GetIOManager();
    qslary::Timer::ptr timer;
    std::weak_ptr<timer_info> winfo(tinfo);
    if (to != (uint64_t)-1) {
      timer = iomanager->addConditionTimer(
          to,
          [winfo, fd, iomanager, event]() {
            auto t = winfo.lock();
            if (!t || t->cancelled)
              return;
            t->cancelled = ETIMEDOUT;
            iomanager->cancelEvent(fd, (qslary::IOManager::Event)(event));
          },
          winfo);
    }

    int rt = iomanager->addEvent(fd, (qslary::IOManager::Event)(event));
    if (rt) {
      QSLARY_LOG_INFO(g_logger)
          << hook_fun_name << " ===================addEvent(" << fdctx << ","
          << event << ")";
      if (timer) {
        timer->cancel();
      }
      return -1;
    } else {
      qslary::Fiber::YieldToHold();
      if (timer) {
        timer->cancel();
      }
      if (tinfo->cancelled) {
        errno = tinfo->cancelled;
        return -1;
      }
      goto retry;
    }
  }
  return n;
}

} // namespace qslary

extern "C" {

#define XX(name) name##_fun name##_f = nullptr;
HOOK_FUNCTION(XX);
#undef XX

/* ------------------------ HOOK Function 实现 ------------------------ */

unsigned int sleep(unsigned int seconds) {
  if (qslary::thread_hook_enable == false) {
    return sleep_f(seconds);
  }
  qslary::Fiber::ptr fiber = qslary::Fiber::GetThreadCurrentFiber();
  qslary::IOManager *iomanager = qslary::IOManager::GetIOManager();
  printf("\n=========hook sleep =========\n");
  iomanager->addTimer(seconds * 1000,
                      [iomanager, fiber]() { iomanager->scheduler(fiber); });
  qslary::Fiber::YieldToHold();
  return 0;
}

int usleep(useconds_t usec) {
  if (qslary::thread_hook_enable == false) {
    return usleep_f(usec);
  }
  qslary::Fiber::ptr fiber = qslary::Fiber::GetThreadCurrentFiber();
  qslary::IOManager *iomanager = qslary::IOManager::GetIOManager();
  iomanager->addTimer(usec / 1000,
                      [iomanager, fiber]() { iomanager->scheduler(fiber); });
  printf("\n=========hook usleep =========\n");
  qslary::Fiber::YieldToHold();
  return 0;
}

int nanaosleep(const struct timespec *req, struct timespec *res) {
  if (qslary::thread_hook_enable == false) {
    return nanaosleep_f(req, res);
  }
  int timeout_ms = req->tv_sec * 1000 + req->tv_nsec / 1000 / 1000;
  qslary::Fiber::ptr fiber = qslary::Fiber::GetThreadCurrentFiber();
  qslary::IOManager *iomanager = qslary::IOManager::GetIOManager();
  iomanager->addTimer(timeout_ms,
                      [iomanager, fiber]() { iomanager->scheduler(fiber); });
  qslary::Fiber::YieldToHold();
  return 0;
}

int socket(int domain, int type, int protocol) {
  if (qslary::thread_hook_enable == false)
    return socket_f(domain, type, protocol);
  // printf("------socket-----\n");
  int fd = socket_f(domain, type, protocol);
  if (fd == -1)
    return fd;
  // printf("------socket-----\n");
  qslary::FdMgr::getInstance()->getFdContext(fd, true);
  return fd;
}

int connect_with_timeout(int sockfd, const struct sockaddr *addr,
                         socklen_t addrlen, uint64_t timeout_ms) {
  if (qslary::thread_hook_enable == false)
    return connect_f(sockfd, addr, addrlen);

  qslary::FdContext::ptr fdctx =
      qslary::FdMgr::getInstance()->getFdContext(sockfd);
  if (!fdctx) {
    printf("---------------------in this EBADF---------------------------\n");
    errno = EBADF;
    return -1;
  }

  if (!fdctx || fdctx->isClosed()) {
    printf("+++++++++++++++++++++++++in this EBADF+++++++++++++++++++++++\n");
    errno = EBADF;
    return -1;
  }
  if (fdctx->isSocket() == false || fdctx->getUserNonBlock()) {
    printf("=======================in this SCOPE========================\n");
    return connect_f(sockfd, addr, addrlen);
  }
  int n = connect_f(sockfd, addr, addrlen);
  printf("\n\n\n\nconnect_f %d errno = %d\n\n\n\n\n", n, errno);
  if (n == 0)
    return 0;
  // TODO
  if (n != -1 || errno != EINPROGRESS)
    return n;
  qslary::IOManager *iomanager = qslary::IOManager::GetIOManager();
  qslary::Timer::ptr timer;
  std::shared_ptr<qslary::timer_info> tinfo(new qslary::timer_info);
  std::weak_ptr<qslary::timer_info> winfo(tinfo);
  // printf("timerout_ms is %ld\n",timeout_ms);
  if (timeout_ms != (uint64_t)(-1)) {
    timer = iomanager->addConditionTimer(
        timeout_ms,
        [winfo, sockfd, iomanager]() {
          auto t = winfo.lock();
          printf("\n\n==================执行了timer的条件超时方法\n\n");
          if (!t || t->cancelled) {
            return;
          }
          printf("Connect_with_timeout ConditionTimer timeout\n");
          t->cancelled = ETIMEDOUT;
          iomanager->cancelEvent(sockfd, qslary::IOManager::Event::WRITE);
        },
        winfo);
  }
  int rt = iomanager->addEvent(sockfd, qslary::IOManager::Event::WRITE);
  if (rt == 0) {
    qslary::Fiber::YieldToHold();
    printf("\nYieldswapin\n\n");
    if (timer) {
      timer->cancel();
    }
    if (tinfo->cancelled) {
      errno = tinfo->cancelled;
      return -1;
    }
  } else {
    if (timer)
      timer->cancel();
    assert(false);
  }
  int error = 0;
  socklen_t len = sizeof(int);
  if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) == -1)
    return -1;
  if (error == 0)
    return 0;
  else {
    errno = error;
    return -1;
  }
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
  return connect_with_timeout(sockfd, addr, addrlen, qslary::s_connect_timeout);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen) {
  int fd = do_io(s, accept_f, "accept", qslary::IOManager::Event::READ,
                 SO_RCVTIMEO, addr, addrlen);
  if (fd >= 0)
    qslary::FdMgr::getInstance()->getFdContext(fd, true);
  return fd;
}

int close(int fd) {
  if (qslary::thread_hook_enable == false)
    return close_f(fd);
  qslary::FdContext::ptr ctx = qslary::FdMgr::getInstance()->getFdContext(fd);
  if (ctx) {
    auto iomanager = qslary::IOManager::GetIOManager();
    if (iomanager) {
      iomanager->cancelAll(fd);
    }
    qslary::FdMgr::getInstance()->delFdContext(fd);
  }
  return close_f(fd);
}

// template for read & write

ssize_t read(int fd, void *buf, size_t count) {
  return do_io(fd, read_f, "read", qslary::IOManager::Event::READ, SO_RCVTIMEO,
               buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt) {
  return do_io(fd, readv_f, "readv", qslary::IOManager::Event::READ,
               SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags) {
  return do_io(sockfd, recv_f, "recv", qslary::IOManager::Event::READ,
               SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                 struct sockaddr *src_addr, socklen_t *addrlen) {
  return do_io(sockfd, recvfrom_f, "recvfrom", qslary::IOManager::Event::READ,
               SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags) {
  return do_io(sockfd, recvmsg_f, "recvmsg", qslary::IOManager::Event::READ,
               SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count) {
  return do_io(fd, write_f, "write", qslary::IOManager::Event::WRITE,
               SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt) {
  return do_io(fd, writev_f, "writev", qslary::IOManager::Event::WRITE,
               SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags) {
  return do_io(s, send_f, "send", qslary::IOManager::Event::WRITE, SO_SNDTIMEO,
               msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags,
               const struct sockaddr *to, socklen_t tolen) {
  return do_io(s, sendto_f, "sendto", qslary::IOManager::Event::WRITE,
               SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags) {
  return do_io(s, sendmsg_f, "sendmsg", qslary::IOManager::Event::WRITE,
               SO_SNDTIMEO, msg, flags);
}

/*         ------------------------- fcntl相关  ----------------------   */

int fcntl(int fd, int cmd, ... /* arg */) {
  va_list va;
  va_start(va, cmd);
  switch (cmd) {
  case F_SETFL: {
    int arg = va_arg(va, int);
    va_end(va);
    qslary::FdContext::ptr ctx = qslary::FdMgr::getInstance()->getFdContext(fd);
    if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
      return fcntl_f(fd, cmd, arg);
    }
    ctx->setUserNonBlock(arg & O_NONBLOCK);
    if (ctx->getSysNonBlock()) {
      arg |= O_NONBLOCK;
    } else {
      arg &= ~O_NONBLOCK;
    }
    return fcntl_f(fd, cmd, arg);
  } break;
  case F_GETFL: {
    va_end(va);
    int arg = fcntl_f(fd, cmd);
    qslary::FdContext::ptr ctx = qslary::FdMgr::getInstance()->getFdContext(fd);
    if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
      return arg;
    }
    if (ctx->getUserNonBlock()) {
      return arg | O_NONBLOCK;
    } else {
      return arg & ~O_NONBLOCK;
    }
  } break;
  case F_DUPFD:
  case F_DUPFD_CLOEXEC:
  case F_SETFD:
  case F_SETOWN:
  case F_SETSIG:
  case F_SETLEASE:
  case F_NOTIFY:
#ifdef F_SETPIPE_SZ
  case F_SETPIPE_SZ:
#endif
  {
    int arg = va_arg(va, int);
    va_end(va);
    return fcntl_f(fd, cmd, arg);
  } break;
  case F_GETFD:
  case F_GETOWN:
  case F_GETSIG:
  case F_GETLEASE:
#ifdef F_GETPIPE_SZ
  case F_GETPIPE_SZ:
#endif
  {
    va_end(va);
    return fcntl_f(fd, cmd);
  } break;
  case F_SETLK:
  case F_SETLKW:
  case F_GETLK: {
    struct flock *arg = va_arg(va, struct flock *);
    va_end(va);
    return fcntl_f(fd, cmd, arg);
  } break;
  case F_GETOWN_EX:
  case F_SETOWN_EX: {
    struct f_owner_exlock *arg = va_arg(va, struct f_owner_exlock *);
    va_end(va);
    return fcntl_f(fd, cmd, arg);
  } break;
  default:
    va_end(va);
    return fcntl_f(fd, cmd);
  }
}

int ioctl(int fd, unsigned long request, ...) {
  va_list va;
  va_start(va, request);
  void *arg = va_arg(va, void *);
  va_end(va);

  if (FIONBIO == request) {
    bool user_nonblock = !!*(int *)arg;
    qslary::FdContext::ptr ctx = qslary::FdMgr::getInstance()->getFdContext(fd);
    if (!ctx || ctx->isClosed() || !ctx->isSocket()) {
      return ioctl_f(fd, request, arg);
    }
    ctx->setUserNonBlock(user_nonblock);
  }
  return ioctl_f(fd, request, arg);
}

int getsockopt(int sockfd, int level, int optname, void *optval,
               socklen_t *optlen) {
  return getsockopt_f(sockfd, level, optname, optval, optlen);
}

int setsockopt(int sockfd, int level, int optname, const void *optval,
               socklen_t optlen) {
  if (qslary::thread_hook_enable == false)
    return setsockopt_f(sockfd, level, optname, optval, optlen);

  // TODO
  if (level == SOL_SOCKET) {
    if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
      qslary::FdContext::ptr fdctx =
          qslary::FdMgr::getInstance()->getFdContext(sockfd);
      if (fdctx) {
        const timeval *v = (const timeval *)optval;
        fdctx->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
      }
    }
  }
  return setsockopt_f(sockfd, level, optname, optval, optlen);
}

} // extern "C"

// typedef unsigned int (*sleep_fun)(unsigned int seconds);
// extern sleep_fun sleep_f;

// typedef int (*usleep_fun)(useconds_t usec);
// extern usleep_fun usleep_f;

// typedef int (*nanaosleep_fun)(const struct timespec* reg, struct timespec*
// rem); extern nanaosleep_fun nanaosleep_f;

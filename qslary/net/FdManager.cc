#include "qslary/net/FdManager.h"
#include "qslary/base/Hook.h"
#include "qslary/base/Mutex.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace qslary
{

FdContext::FdContext(int fd)
    : isInit_(false), isSocket_(false), sysNonblock_(false),
      userNonblock_(false), isClosed_(false), fd_(fd),
      recvTimeout_(-1), sendTimeout_(-1)
{
  init();
}

FdContext::~FdContext()
{
  
}
bool FdContext::init()
{
  if (isInit())
  {
    return true;
  }
  recvTimeout_ = -1;
  sendTimeout_ = -1;
  struct stat fd_stat;
  if (fstat(fd_, &fd_stat) == -1) // 失败
  {
    isInit_ = false;
    isSocket_=false;
  }
  else
  {
    isInit_ = true;
    isSocket_=S_ISSOCK(fd_stat.st_mode);
  }
  if (isSocket_)
  {
    int flags = fcntl_f(fd_, F_GETFL, 0);
    if (!(flags & O_NONBLOCK)) // 是阻塞
    {
      fcntl_f(fd_,F_SETFL,flags|O_NONBLOCK);
    }
    sysNonblock_=true;
  }
  else
  {
    sysNonblock_=false;
  }
  userNonblock_ = false;
  isClosed_ = false;
  return isInit_;
}

// bool FdContext::close()
// {
  
// }

void FdContext::setTimeout(int type, uint64_t v)
{
  if (type == SO_RCVTIMEO)
  {
    recvTimeout_=v;
  }
  else
  {
    sendTimeout_=v;
  }
}
uint64_t FdContext::getTimeout(int type)
{
  if (type == SO_RCVTIMEO)
  {
    return recvTimeout_;
  }
  return sendTimeout_;
}

FdContextManager::FdContextManager()
{
  fds_.resize(64);
}
FdContextManager::~FdContextManager()
{
  
}

FdContext::ptr FdContextManager::getFdContext(int fd, bool auto_creat)
{
  if (fd == -1)
    return nullptr;
  {
  qslary::ReadLockGuard lock(mutex_);
  if ((size_t)fd < fds_.size())
    if (fds_[fd] || auto_creat == false)
      return fds_[fd];
  }
  if (auto_creat)
  {
    qslary::WriteLockGuard lock(mutex_);
    if (fds_.size() <= (size_t)fd)
      fds_.resize(fd * 1.5);
    FdContext::ptr ctx(new FdContext(fd));
    fds_[fd] = ctx;
    return ctx;
  }
  return nullptr;
}

void FdContextManager::delFdContext(int fd)
{
  qslary::WriteLockGuard lock(mutex_);
  if ((size_t)fd > fds_.size())
  {
    return;
  }
  fds_[fd].reset();
}

} // namesapce qslary 
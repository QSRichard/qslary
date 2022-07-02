#ifndef __QSLARY_FD_MANAGER_H_
#define __QSLARY_FD_MANAGER_H_

#include "qslary/base/Mutex.h"
#include "qslary/base/singleton.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace qslary
{
class IOManager;

class FdContext
{
public:
  typedef std::shared_ptr<FdContext> ptr;

  FdContext(int fd);
  ~FdContext();
  bool init();
  bool isInit() const
  {
    return isInit_;
  }

  bool isSocket() const
  {
    return isSocket_;
  }

  bool isClosed() const 
  {
    return isClosed_;
  }

  // bool close();

  void setUserNonBlock(bool v)
  {
    userNonblock_ = v;
  }
  bool getUserNonBlock() const
  {
    return userNonblock_;
  }

  void setSysNonBlock(bool v)
  {
    sysNonblock_=v;
  }

  bool getSysNonBlock() const
  {
    return sysNonblock_;
  }

  void setTimeout(int type, uint64_t v);
  uint64_t getTimeout(int type);
private:
  bool isInit_;
  bool isSocket_;
  bool sysNonblock_;
  bool userNonblock_;
  bool isClosed_;
  int fd_;
  std::uint64_t recvTimeout_;
  uint64_t sendTimeout_;
  qslary::IOManager* iomanager_;
};

class FdContextManager
{
public:
  typedef std::shared_ptr<FdContextManager> ptr;
  FdContextManager();
  ~FdContextManager();

  FdContext::ptr getFdContext(int fd, bool auto_creat = false);
  void delFdContext(int fd);

private:
    qslary::RWMutex mutex_;
    std::vector<FdContext::ptr> fds_;
};

typedef Singleton<FdContextManager> FdMgr;
} // namespace qslary

#endif // __QSLARY_FD_MANAGER_H_
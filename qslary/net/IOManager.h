#ifndef __QSLARY_IOMANAGER_H_
#define __QSLARY_IOMANAGER_H_

#include "qslary/base/Fiber.h"
#include "qslary/base/Mutex.h"
#include "qslary/base/alise_config.h"
#include "qslary/net/FdManager.h"
#include "qslary/net/Scheduler.h"
#include "qslary/net/Timer.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <sys/epoll.h>
namespace qslary
{

struct FdCallback
{
  FdCallback(IOManager *manger) : scheduler(manger){};
  int TriggerEvent(epoll_event &event);

  IOManager *getSchedule() { return scheduler; }
  ReadCallback getReadCallback() { return readCallback_; }
  WriteCallback getWriteCallback() { return writeCallback_; }
  void setReadCallback(ReadCallback newfun) { readCallback_ = newfun; }
  void setWriteCallback(WriteCallback newfun) { writeCallback_ = newfun; }

  IOManager *scheduler;
  bool concernRead = false;
  bool concernWrite = false;
  ReadCallback readCallback_;
  WriteCallback writeCallback_;
};

class IOManager : public Scheduler, public TimerManger
{
  public:
  typedef std::shared_ptr<IOManager> ptr;
  typedef std::function<void()> ReadCallback;
  typedef std::function<void()> WriteCallback;

  public:
  IOManager(size_t threads = 1, const std::string &name = "");
  ~IOManager();

  bool AddReadEvent(int fd, ReadCallback cb);
  bool AddWriteEvent(int fd, WriteCallback cb);
  bool CancelReadEvent(int fd);
  bool CancelWriteEvent(int fd);
  bool DelFileDesc(int fd);

  bool CancelAll(int fd)
  {
    return CancelReadEvent(fd) && CancelWriteEvent(fd);
  }

  bool IsClean(uint64_t &timeout);
  static IOManager *GetIOManager();

  protected:
  // 继承自Scheduler的虚函数
  virtual void Wakeup() override;
  virtual bool IsClean() override;
  virtual void Idle() override;

  private:
  void ProcessFdEvent(epoll_event &event);
  void UpdateEpoll(int fd, epoll_event &event);

  private:
  int epollfd_ = 0;
  int mWakeupFd = 0;
  std::atomic<size_t> pendingEventCount_ = {0};
  qslary::MutexLock mIoMangerMutex;
  std::map<int, FdCallback *> mFdGroups;
};
} // namespace qslary

#endif
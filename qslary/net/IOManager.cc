#include "qslary/base/Fiber.h"
#include "qslary/base/Logger.h"
#include "qslary/base/Mutex.h"
#include "qslary/net/FdManager.h"
#include "qslary/net/IOManager.h"
#include "qslary/net/Scheduler.h"
#include <cstdint>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <system_error>
#include <unistd.h>

namespace qslary {

static Logger::ptr g_logger = QSLARY_LOG_NAME("system");
static const int MAX_TIMEOUT = 5000;

IOManager::IOManager(size_t threadsNum_, const std::string &name_)
    : Scheduler(threadsNum_, name_), TimerManger()
{
  epollfd_ = epoll_create(5000);
  mWakeupFd = ::eventfd(0, 0);
  epoll_event event;
  memset(&event, 0, sizeof(epoll_event));
  event.events = EPOLLIN;
  event.data.fd = mWakeupFd;

  int rt = epoll_ctl(epollfd_, EPOLL_CTL_ADD, mWakeupFd, &event);
  assert(!rt);
  AddReadEvent(timerfd_, std::bind(&IOManager::handleRead, this));
  start();
}

IOManager::~IOManager()
{
  // FIXME 这里应该加锁
  stop();
  close(epollfd_);
  close(mWakeupFd);
  for (auto it = mFdGroups.begin(); it != mFdGroups.end(); it++)
  {
    delete it->second;
  }
  std::cout << "IOManager 析构" << std::endl;
}

bool IOManager::AddReadEvent(int fd, std::function<void()> cb)
{
  MutexLockGuard guard(mIoMangerMutex);
  epoll_event event;
  event.events = EPOLLIN;
  UpdateEpoll(fd, event);
  mFdGroups[fd]->concernRead = true;
  mFdGroups[fd]->readCallback_ = cb;
  return true;
}

bool IOManager::AddWriteEvent(int fd, std::function<void()> cb)
{
  MutexLockGuard guard(mIoMangerMutex);
  epoll_event event;
  event.events = EPOLLOUT;
  UpdateEpoll(fd, event);
  mFdGroups[fd]->concernWrite = true;
  mFdGroups[fd]->writeCallback_ = cb;
  return true;
}

bool IOManager::CancelReadEvent(int fd)
{
  MutexLockGuard guard(mIoMangerMutex);
  auto it = mFdGroups.find(fd);
  if (it == mFdGroups.end() || !it->second->concernRead)
  {
    return true;
  }
  it->second->concernRead = false;
  return true;
}

bool IOManager::CancelWriteEvent(int fd)
{
  MutexLockGuard guard(mIoMangerMutex);
  auto it = mFdGroups.find(fd);
  if (it == mFdGroups.end() || !it->second->concernWrite)
  {
    return true;
  }
  it->second->concernWrite = false;
  return true;
}

bool IOManager::DelFileDesc(int fd)
{
  MutexLockGuard guard(mIoMangerMutex);
  mFdGroups.erase(mFdGroups.find(fd));
  return true;
}

bool IOManager::IsClean(uint64_t &timeout)
{
  return timeout == ~0ull && pendingEventCount_ == 0 && Scheduler::IsClean();
}

IOManager *IOManager::GetIOManager()
{
  return dynamic_cast<IOManager *>(Scheduler::GetThreadScheduler());
}

void IOManager::Wakeup()
{
  if (hasIdleThreads())
  {
    return;
  }
  uint64_t buf = 1;
  write(mWakeupFd, &buf, sizeof(buf));
}

bool IOManager::IsClean()
{
  uint64_t timeout = 0;
  return IsClean(timeout);
}

void IOManager::Idle()
{
  epoll_event *events = new epoll_event[64]();
  std::shared_ptr<epoll_event> shared_events(
      events, [](epoll_event *ptr) { delete[] ptr; });
  while (true)
  {
    int ret = 0;
    while (true && !ret)
    {
      std::cout << "qiaoshuo" << std::endl;
      ret = epoll_wait(epollfd_, events, 64, MAX_TIMEOUT);
    }
    for (int i = 0; i < ret; i++)
    {
      if (events[i].data.fd == mWakeupFd)
      {
        uint64_t dummy;
        read(mWakeupFd, &dummy, sizeof(dummy));
        continue;
      }
      ProcessFdEvent(events[i]);
    }
    Fiber::GetThreadCurrentFiber()->swapOut();
  }
}

void IOManager::UpdateEpoll(int fd, epoll_event &event)
{
  bool modCtl = true;
  event.data.fd = fd;
  if (mFdGroups.find(fd) == mFdGroups.end())
  {
    FdCallback *fdc = new FdCallback(this);
    mFdGroups[fd] = fdc;
    epoll_ctl(epollfd_, EPOLL_CTL_ADD, fd, &event);
    modCtl = false;
  }
  if (modCtl)
  {
    epoll_ctl(epollfd_, EPOLL_CTL_MOD, fd, &event);
  }
}

void IOManager::ProcessFdEvent(epoll_event &event)
{
  if (event.events & (EPOLLERR | EPOLLHUP))
  {
    event.events |= (EPOLLIN | EPOLLOUT);
  }
  int num = mFdGroups[event.data.fd]->TriggerEvent(event);
  pendingEventCount_ -= num;
}

int FdCallback::TriggerEvent(epoll_event &event)
{
  int eventNumber = 0;
  if (concernRead && event.events & EPOLLIN)
  {
    scheduler->scheduler(readCallback_);
    eventNumber++;
  }
  if (concernRead && event.events & EPOLLOUT)
  {
    scheduler->scheduler(writeCallback_);
    eventNumber++;
  }
  return eventNumber;
}
} // namespace qslary

#include "qslary/net/IOManager.h"
#include "qslary/net/FdManager.h"
#include "qslary/base/Fiber.h"
#include "qslary/base/Logger.h"
#include "qslary/base/Mutex.h"

#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <system_error>
#include <unistd.h>

namespace qslary
{

  static Logger::ptr g_logger=QSLARY_LOG_NAME("system");

void IOManager::FdContext::triggerEvent(IOManager::Event event)
{

  assert(events_ & event);
  // TODO
  QSLARY_LOG_DEBUG(g_logger)<<"进入triggerEvent";
  events_ = (Event)(events_ & ~event);

  if (event == IOManager::Event::READ)
  {
    scheduler->scheduler(readCallback_);
  }
  else if (event == IOManager::Event::WRITE)
  {
    scheduler->scheduler(writeCallback_);
  }
  // FIXME 
  // scheduler = nullptr;
  return;
}

IOManager::IOManager(size_t threadsNum_, const std::string& name_)
    : Scheduler(threadsNum_, name_)
{

  epollfd_ = epoll_create(5000);
  assert(epollfd_ > 0);
  int rt = pipe(tickleFds_);
  assert(!rt);
  epoll_event event;
  memset(&event, 0, sizeof(epoll_event));
  event.events = EPOLLIN;
  event.data.fd = tickleFds_[0];
  rt=fcntl(tickleFds_[0], F_SETFL,O_NONBLOCK);
  rt = epoll_ctl(epollfd_, EPOLL_CTL_ADD, tickleFds_[0], &event);
  assert(!rt);
  contextResize(64);
  start();
}

IOManager::~IOManager()
{
  // Scheduler::stop()
  stop();
  close(epollfd_);
  close(tickleFds_[0]);
  close(tickleFds_[1]);
  for (size_t i = 0; i < fdcontexts_.size(); i++)
  {
    if (fdcontexts_[i])
    {
      delete fdcontexts_[i];
    }
  }
  std::cout << "IOManager 析构" << std::endl;
}

void IOManager::contextResize(size_t size)
{
  fdcontexts_.resize(size);
  for (size_t i = 0; i < fdcontexts_.size(); i++)
  {
    if (!fdcontexts_[i])
    {
      fdcontexts_[i] = new FdContext;
      fdcontexts_[i]->fd = i;
    }
  }
}

int IOManager::addEvent(int fd, Event event, std::function<void()> cb)
{
  if (cb == nullptr)
    assert(false);

  FdContext* fd_ctx = nullptr;

  {
    qslary::MutexLockGuard lock(mutex_);
    if ((int)fdcontexts_.size() > fd)
    {
      fd_ctx = fdcontexts_[fd];
    }
    else
    {
      contextResize(fd * 1.5);
      fd_ctx = fdcontexts_[fd];
    }
  }

  qslary::MutexLockGuard lock(fd_ctx->mutex_);

  if (fd_ctx->events_ & event)
  {
    assert(false);
  }

  int op = fd_ctx->events_ ? EPOLL_CTL_MOD : EPOLL_CTL_ADD;
  epoll_event epoll_e;
  epoll_e.events = fd_ctx->events_ | event;
  epoll_e.data.ptr = (void*)fd_ctx;

  int rt = epoll_ctl(epollfd_, op, fd, &epoll_e);

  if (rt)
  {
    // TODO logging error
    std::cout << "epoll ctl error" << rt;
    return -1;
  }
  ++pendingEventCount_;

  // 修改关注的事件
  fd_ctx->events_ = (Event)(fd_ctx->events_ | event);

  // assert(!event_ctx.scheduler && !event_ctx.cb && !event_ctx.fiber);
  fd_ctx->scheduler = qslary::Scheduler::GetThreadScheduler();
  if (event == IOManager::Event::READ)
    fd_ctx->readCallback_ = cb;
  else if (event == IOManager::Event::WRITE)
    fd_ctx->writeCallback_ = cb;
  return 0;
}

bool IOManager::delEvent(int fd, Event event)
{
  FdContext* fd_ctx = nullptr;
    qslary::MutexLockGuard lock(mutex_);
    if (fdcontexts_.size() <= (size_t)fd)
    {
      return false;
    }
    fd_ctx = fdcontexts_[fd];
    if (!(fd_ctx->events_ & event))
    {
      return false;
    }

  Event new_event = (Event)(fd_ctx->events_ & ~event);
  int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epoll_e;
  epoll_e.events = new_event;
  epoll_e.data.ptr = fd_ctx;
  int ret = epoll_ctl(epollfd_, op, fd, &epoll_e);
  if (ret)
  {
    // TODO logging error
    assert(false);
    return false;
  }
  --pendingEventCount_;
  fd_ctx->events_ = new_event;
  if (event == IOManager::Event::READ)
    fd_ctx->readCallback_ = nullptr;
  else if (event == IOManager::Event::WRITE)
    fd_ctx->writeCallback_ = nullptr;
  return true;
}


bool IOManager::cancelEvent(int fd, Event event)
{
    FdContext* fd_ctx = nullptr;
    qslary::MutexLockGuard lock(mutex_);
    if (fdcontexts_.size() <= (size_t)fd)
    {
      return false;
    }
    fd_ctx = fdcontexts_[fd];
  if (!(fd_ctx->events_ & event))
  {
    return false;
  }

  Event new_event = (Event)(fd_ctx->events_ & ~event);
  int op = new_event ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
  epoll_event epoll_e;
  epoll_e.events = new_event;
  epoll_e.data.ptr = fd_ctx;
  int ret = epoll_ctl(epollfd_, op, fd, &epoll_e);
  if (ret)
  {
    // TODO logging error
    assert(false);
    return false;
  }
  fd_ctx->triggerEvent(event);
  --pendingEventCount_;
  return true;
}


bool IOManager::cancelAll(int fd)
{
  FdContext* fd_ctx = nullptr;
  {
    qslary::MutexLockGuard lock(mutex_);
    if (fdcontexts_.size() <= (size_t)fd)
    {
      return false;
    }
    fd_ctx = fdcontexts_[fd];
  }
  qslary::MutexLockGuard lock(mutex_);
  if (!fd_ctx->events_)
  {
    return false;
  }

  int op = EPOLL_CTL_DEL;
  epoll_event epoll_e;
  epoll_e.events = 0;
  epoll_e.data.ptr = fd_ctx;
  int ret = epoll_ctl(epollfd_, op, fd, &epoll_e);
  if (ret)
  {
    // TODO logging error
    assert(false);
    return false;
  }

  if (fd_ctx->events_ & Event::READ)
  {
    fd_ctx->triggerEvent(Event::READ);
    --pendingEventCount_;
  }
  if (fd_ctx->events_ & Event::WRITE)
  {
    fd_ctx->triggerEvent(Event::WRITE);
    --pendingEventCount_;
  }
  return true;
}

bool IOManager::stopping(uint64_t &timeout){
  // timeout=getNextTimer();
  return timeout==~0ull && pendingEventCount_==0 && Scheduler::stopping();
}

IOManager* IOManager::GetIOManager()
{
  return dynamic_cast<IOManager*>(Scheduler::GetThreadScheduler());
}

void IOManager::tickle()
{
  if (hasIdleThreads())
  {
    return;
  }
  int rt = write(tickleFds_[1], "T", 1);
  assert(rt == 1);
}


bool IOManager::stopping()
{
  uint64_t timeout=0;

  return stopping(timeout);
}

void IOManager::run()
{

  Fiber::ptr idle_fiber(new Fiber(std::bind(&IOManager::idle, this)));
  Fiber::ptr cb_fiber;
  Fiber fiber;
  while (true)
  {
    bool tickle_me = false;
    bool is_active = false;
    {
      qslary::MutexLockGuard lock(mutex_);
      auto it = pendingFunctors_.begin();
      while (it != pendingFunctors_.end())
      {
        if (it->getState() == Fiber::State::EXEC)
        {
          ++it;
          continue;
        }
        fiber = *it;
        pendingFunctors_.erase(it);
        ++activeThreadCount_;
        is_active = true;
        break;
      }
      tickle_me |= it != pendingFunctors_.end();
    }

    if (tickle_me)
    {
      tickle();
    }

    // ft.fiber 为真
    if (fiber.getState() != Fiber::State::TERM && fiber.getState() != Fiber::State::EXCEPTION)
    {

      // SwapIn 将当前协程与Scheduler::GetMainFiber切换
      fiber.swapIn();
      --activeThreadCount_;
      if (fiber.getState() == Fiber::State::READY)
      {
        scheduler(fiber.cb_);
      }
      else if (fiber.getState() != Fiber::State::TERM && fiber.getState() != Fiber::State::EXCEPTION)
      {
        fiber.state_ = Fiber::State::HOLD;
      }
      fiber.reset(nullptr);
    } // ft.fiber 为真

    else // 进入idle
    {
      if (is_active)
      {
        --activeThreadCount_;
        continue;
      }
      if (idle_fiber->getState() == Fiber::State::TERM)
      {
        QSLARY_LOG_INFO(g_logger) << " idle fiber 结束";
        break;
      }
      ++idleThreadCount_;
      idle_fiber->swapIn();
      --idleThreadCount_;
      if (idle_fiber->getState() != Fiber::State::TERM && idle_fiber->getState() != Fiber::State::EXCEPTION)
      {
        idle_fiber->state_ = Fiber::State::HOLD;
      }
    }
  }
}

void IOManager::idle()
{

  epoll_event* events = new epoll_event[64]();
  std::shared_ptr<epoll_event> shared_events(events, [](epoll_event* ptr) { delete[] ptr; });
  while (true)
  {
    uint64_t next_time = 0;
    if (stopping(next_time))
    {
      std::cout << "stoping " << std::endl;
      break;
    }

    static const int MAX_TIMEOUT=1000;
    int ret = 0;
    while (true)
    {
      if (next_time != ~0ull)
        next_time = (int)next_time > MAX_TIMEOUT ? MAX_TIMEOUT : next_time;
      else
        next_time = MAX_TIMEOUT;

      ret = epoll_wait(epollfd_, events, 64, (int)next_time);
      if (ret >= 0)
        break;
    }
    std::vector<std::function<void()>> callerbacks;
    // listExpiredCallerBack(callerbacks);
    if (!callerbacks.empty())
    {
      scheduler(callerbacks.begin(),callerbacks.end());
      for(auto& i:callerbacks){
        scheduler(i);
      }
      callerbacks.clear();
    }
    for (int i = 0; i < ret; i++)
    {
      epoll_event& event = events[i];
      if (event.data.fd == tickleFds_[0])
      {
        u_int8_t dummy[256];
        // QSLARY_LOG_DEBUG(g_logger)<<"调用read";
        // FIXME
        while (read(tickleFds_[0], &dummy, sizeof(dummy)) > 0)
          // QSLARY_LOG_DEBUG(g_logger) <<dummy;
              read(tickleFds_[0],&dummy,1);
          continue;
      }// 判断处理tickleFd

      // 判断处理epoll事件
      FdContext* fd_ctx = (FdContext*)event.data.ptr;
      qslary::MutexLockGuard lock(fd_ctx->mutex_);
      if (event.events & (EPOLLERR | EPOLLHUP))
      {
        event.events |= (EPOLLIN | EPOLLOUT);
      }
      // 判断处理 real_event事件
      int real_events = IOManager::Event::NONE;
      if (event.events & EPOLLOUT)
      {
        real_events |= IOManager::Event::WRITE;
      }
      if (event.events & EPOLLIN)
      {
        real_events |= IOManager::Event::READ;
      }
      // fd 关切的事件 & 发生的事件为NONE
      if ((fd_ctx->events_ & real_events) == NONE)
      {
        continue;
      }
      // 更新epoll事件
      int left_events = (fd_ctx->events_ & ~real_events);
      int op = left_events ? EPOLL_CTL_MOD : EPOLL_CTL_DEL;
      event.events = left_events;
      int temp = epoll_ctl(epollfd_, op, fd_ctx->fd, &event);
      if (temp)
      {
        // TODO logging error
        assert(false);
      }
      // 处理事件回调函数
      if (real_events & IOManager::Event::READ)
      {
        fd_ctx->triggerEvent(IOManager::Event::READ);
        --pendingEventCount_;
      }
      if (real_events & IOManager::Event::WRITE)
      {
        fd_ctx->triggerEvent(IOManager::Event::WRITE);
        --pendingEventCount_;
      }
    }
    // TAG 
    // if(ret>0){
    //   QSLARY_LOG_DEBUG(g_logger)<<"事件处理完成";
    // }

    Fiber::ptr cur = Fiber::GetThreadCurrentFiber();
    auto raw_ptr = cur.get();
    cur.reset();
    raw_ptr->swapOut();
  }
}

} // namespace qslary
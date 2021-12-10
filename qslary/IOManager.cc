
#include "IOManager.h"
#include "logger.h"

#include <fcntl.h>
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>

namespace qslary
{

  static Logger::ptr g_logger=QSLARY_LOG_NAME("system");

IOManager::FdContext::EventContext& IOManager::FdContext::getEventContext(IOManager::Event event)
{
  switch (event)
  {
  case IOManager::Event::READ:
    return read;
  case IOManager::Event::WRITE:
    return write;
  default:
    assert(false);
  }
}

void IOManager::FdContext::resetEventContext(EventContext& new_ctx)
{
  new_ctx.cb = nullptr;
  new_ctx.fiber.reset();
  new_ctx.scheduler = nullptr;
  return;
}

void IOManager::FdContext::triggerEvent(IOManager::Event event)
{
  
  assert(events_ & event);
  QSLARY_LOG_DEBUG(g_logger)<<"进入triggerEvent";
  events_ = (Event)(events_ & ~event);
  EventContext& ctx = getEventContext(event);
  if (ctx.cb)
  {
    ctx.scheduler->scheduler(&ctx.cb);
  }
  else
  {
    ctx.scheduler->scheduler(&ctx.fiber);
  }
  ctx.scheduler = nullptr;
  return;
}

IOManager::IOManager(size_t threadsNum_, bool useCaller_, const std::string& name_)
    : Scheduler(threadsNum_, useCaller_, name_)
{

  epollfd_ = epoll_create(5000);
  assert(epollfd_ > 0);
  int rt = pipe(tickleFds_);
  assert(!rt);
  epoll_event event;
  memset(&event, 0, sizeof(epoll_event));
  event.events = EPOLLIN;
  event.data.fd = tickleFds_[0];
  rt = epoll_ctl(epollfd_, EPOLL_CTL_ADD, tickleFds_[0], &event);
  assert(!rt);
  fdcontexts_.resize(64);
  QSLARY_LOG_DEBUG(g_logger)<<"IOManager::IOManager() 在这里启动Scheduler::start()方法";
  start();
}

IOManager::~IOManager()
{
  std::cout<<"IOManager 析构"<<std::endl;
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
  FdContext* fd_ctx = nullptr;
  {
    qslary::ReadLockGuard lock(mutex_);
    if (fdcontexts_.size() > (size_t)fd)
    {
      fd_ctx = fdcontexts_[fd];
    }
  }
  {
    qslary::WriteLockGuard lock(mutex_);
    contextResize((size_t)fd * 1.5);
    fd_ctx = fdcontexts_[fd];
    // FIXME add event
  }

  qslary::WriteLockGuard lock(mutex_);
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
    std::cout << "epoll ctl"
              << "error";
    return -1;
  }
  ++pendingEventCount_;
  fd_ctx->events_ = (Event)(fd_ctx->events_ | event);
  FdContext::EventContext& event_ctx = fd_ctx->getEventContext(event);

  assert(!event_ctx.scheduler && !event_ctx.cb && !event_ctx.fiber);
  event_ctx.scheduler = qslary::Scheduler::GetThreadScheduler();
  if (cb)
  {
    event_ctx.cb.swap(cb);
  }
  else
  {
    event_ctx.fiber = qslary::Fiber::GetThreadCurrentFiber();
    assert(event_ctx.fiber->getState() == qslary::Fiber::State::EXEC);
  }
  return 0;
}

bool IOManager::delEvent(int fd, Event event)
{
  FdContext* fd_ctx = nullptr;
  {
    qslary::ReadLockGuard lock(mutex_);
    if (fdcontexts_.size() <= (size_t)fd)
    {
      return false;
    }
    fd_ctx = fdcontexts_[fd];
  }
  qslary::WriteLockGuard lock(mutex_);
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
  FdContext::EventContext& event_ctx = fd_ctx->getEventContext(event);
  fd_ctx->resetEventContext(event_ctx);
  return true;
}
bool IOManager::cancelEvent(int fd, Event event)
{
  FdContext* fd_ctx = nullptr;
  {
    qslary::ReadLockGuard lock(mutex_);
    if (fdcontexts_.size() <= (size_t)fd)
    {
      return false;
    }
    fd_ctx = fdcontexts_[fd];
  }
  qslary::WriteLockGuard lock(mutex_);
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
    qslary::ReadLockGuard lock(mutex_);
    if (fdcontexts_.size() <= (size_t)fd)
    {
      return false;
    }
    fd_ctx = fdcontexts_[fd];
  }
  qslary::WriteLockGuard lock(mutex_);
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
  timeout=getNextTimer();
  // timers为空 并且 penddingEventCount_=0 并且 fibers.empty()=true 并且activeThreadCount=0
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
void IOManager::idle()
{
  QSLARY_LOG_DEBUG(g_logger)<<"entry IOManager::idle()";

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
    int ret=0;
    while(true){
      next_time=(int)next_time>MAX_TIMEOUT?MAX_TIMEOUT:next_time;
      ret=epoll_wait(epollfd_, events, 64, (int)next_time);
      if(ret<0 && errno==EINTR){
        // TODO logging error
        continue;
      }
      else{
        break;
      }
    }
    // FIXME callerbacks 内存以及复制的开销
    std::vector<std::function<void()>> callerbacks;
    listExpiredCallerBack(callerbacks);
    if(!callerbacks.empty()){
      QSLARY_LOG_DEBUG(g_logger)<<"listExpiredCallBack()不为空 size为"<<callerbacks.size();
      // scheduler(callerbacks.begin(),callerbacks.end());
      for(auto& i:callerbacks){
        scheduler(i);
      }
      callerbacks.clear();
    }

    if(ret>0){
      QSLARY_LOG_DEBUG(g_logger)<<"处理事件 "<<"事件数量为 "<<ret;
    }
    // 循环处理事件
    for (int i = 0; i < ret; i++)
    {
      epoll_event& event = events[i];
      // 判断处理tickleFd
      if (event.data.fd == tickleFds_[0])
      {
        u_int8_t dummy;
        while (read(tickleFds_[0], &dummy, 1) == 1)
          ;
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
    if(ret>0){
      QSLARY_LOG_DEBUG(g_logger)<<"事件处理完成";
    }

    Fiber::ptr cur = Fiber::GetThreadCurrentFiber();
    auto raw_ptr = cur.get();
    cur.reset();
    
    QSLARY_LOG_DEBUG(g_logger)<<"willing swapOut";
    raw_ptr->swapOut();
  }
}

void IOManager::onTimerInsertAtFront(){
  QSLARY_LOG_DEBUG(g_logger)<<"onTimerInsertAtFront";
  tickle();

}
} // namespace qslary
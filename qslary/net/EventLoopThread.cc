#include "qslary/net/EventLoopThread.h"
#include "qslary/net/EventLoop.h"
#include <functional>

namespace qslary
{

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb, const std::string& name)
    : mutex_(), loop_(nullptr), exiting_(false), thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      cond_(mutex_), callback_(cb)
{
}

EventLoopThread::~EventLoopThread() {}

EventLoop* EventLoopThread::startLoop()
{

  EventLoop* retLoop = nullptr;
  thread_.start();
  MutexLockGuard lock(mutex_);
  while (loop_ == nullptr)
  {
    cond_.wait();
  }
  retLoop = loop_;
  return retLoop;

}

void EventLoopThread::threadFunc()
{

  EventLoop loop;
  if (callback_)
  {
    callback_(&loop);
  }

  {
    MutexLockGuard lock(mutex_);
    loop_ = &loop;
    cond_.notify();
  }

  loop.loop();

  // 退出时
  MutexLockGuard lock(mutex_);
  loop_ = nullptr;
}


} // namespace qslary
#include "qslary/net/EventLoop.h"
#include "qslary/net/Channel.h"
#include "qslary/net/Poller.h"
#include "qslary/base/Fiber.h"

#include "qslary/net/Socket.h"
#include <algorithm>
#include <memory>
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include <utility>

namespace qslary
{

__thread EventLoop* t_loopInThisThread = 0;

const int kPollTimeMs = 10000;

int createEventfd()
{
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0)
  {
    abort();
  }
  return evtfd;
}

#pragma GCC diagnostic ignored "-Wold-style-cast"
class IgnoreSigPipe
{
public:
  IgnoreSigPipe()
  {
    ::signal(SIGPIPE, SIG_IGN);
    // LOG_TRACE << "Ignore SIGPIPE";
  }
};
#pragma GCC diagnostic error "-Wold-style-cast"

IgnoreSigPipe initObj;

EventLoop* EventLoop::getEventLoopOfCurrentThread() { return t_loopInThisThread; }

EventLoop::EventLoop()
    : looping_(false), quit_(false), eventHandling_(false), callingPendingFunctors_(false), iteration_(0),
      threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)), timerManger_(new TimerManger(this)),
      wakeupFd_(createEventfd()), wakeupChannel_(new Channel(this, wakeupFd_)), currentActiveChannel_(NULL)
{

  if (t_loopInThisThread)
  {
    std::cout << "Another EventLoop " << t_loopInThisThread << " exists in this thread " << threadId_;
  }
  else
  {
    t_loopInThisThread = this;
  }
  wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
  // we are always reading the wakeupfd
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
  std::cout << "EventLoop " << this << " of thread " << threadId_ << " destructs in thread " << CurrentThread::tid();
  wakeupChannel_->disableAll();
  wakeupChannel_->remove();
  ::close(wakeupFd_);
  t_loopInThisThread = NULL;
}

void EventLoop::loop()
{
  assert(!looping_);
  assertInLoopThread();
  looping_ = true;
  quit_ = false; // FIXME: what if someone calls quit() before loop() ?

  while (!quit_)
  {
    activeChannels_.clear();
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    ++iteration_;
    // TODO sort channel by priority
    eventHandling_ = true;
    for (Channel* channel : activeChannels_)
    {
      currentActiveChannel_ = channel;
      currentActiveChannel_->handleEvent(pollReturnTime_);
    }
    currentActiveChannel_ = NULL;
    eventHandling_ = false;
    doPendingFunctors();
  }

  std::cout << "EventLoop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::quit()
{
  quit_ = true;
  // There is a chance that loop() just executes while(!quit_) and exits,
  // then EventLoop destructs, then we are accessing an invalid object.
  // Can be fixed using mutex_ in both places.
  if (!isInLoopThread())
  {
    wakeup();
  }
}

void EventLoop::runInLoop(Functor cb)
{
  if (isInLoopThread())
  {
    cb();
  }
  else
  {
    queueInLoop(std::move(cb));
  }
}

void EventLoop::queueInLoop(Functor cb)
{
  {
    MutexLockGuard lock(mutex_);
    pendingFunctors_.push_back(std::make_shared<Fiber>(std::move(cb)));
  }

  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();
  }
}

size_t EventLoop::queueSize() const
{
  MutexLockGuard lock(mutex_);
  return pendingFunctors_.size();
}

TimerId EventLoop::runAt(Timestamp time, std::function<void()> cb)
{
  return timerManger_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, std::function<void()> cb)
{
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, std::function<void()> cb)
{
  Timestamp time(addTime(Timestamp::now(), interval));
  return timerManger_->addTimer(std::move(cb), time, interval);
}

void EventLoop::cancel(TimerId timerId) { return timerManger_->cancel(timerId); }

void EventLoop::updateChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  if (eventHandling_)
  {
    assert(currentActiveChannel_ == channel ||
           std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
  }
  poller_->removeChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  return poller_->hasChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
  std::cout << "EventLoop::abortNotInLoopThread - EventLoop " << this << " was created in threadId_ = " << threadId_
            << ", current thread id = " << CurrentThread::tid();
}

void EventLoop::wakeup()
{
  //   uint64_t one = 1;
  //   ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
  //   if (n != sizeof one)
  //   {
  //     LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
  //   }
}

void EventLoop::handleRead()
{
  //   uint64_t one = 1;
  //   ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
  //   if (n != sizeof one)
  //   {
  //     LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
  //   }
}

void EventLoop::doPendingFunctors()
{
  std::vector<Fiber::ptr> functors;
  callingPendingFunctors_ = true;

  {
    MutexLockGuard lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (const Fiber::ptr& fiber : functors)
  {
    if (fiber->getState() != Fiber::State::TERM && fiber->getState() != Fiber::State::EXCEPTION)
    {
      // SwapIn 将当前协程与Scheduler::GetMainFiber切换
      fiber->swapIn();
      if (fiber->getState() == Fiber::State::READY)
      {
        queueInLoop(fiber->cb_);
      }
      else if (fiber->getState() != Fiber::State::TERM && fiber->getState() != Fiber::State::EXCEPTION)
      {
        fiber->state_ = Fiber::State::HOLD;
      }
      fiber->reset(nullptr);
    }
    callingPendingFunctors_ = false;
  }
}

// void EventLoop::printActiveChannels() const
// {
// //   for (const Channel* channel : activeChannels_)
// //   {
// // //     LOG_TRACE << "{" << channel->reventsToString() << "} ";
// //   }
// }
} // namespace qslary

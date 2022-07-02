#include "qslary/net/Timer.h"
#include "qslary/base/Logger.h"
#include "qslary/base/types.h"
#include "qslary/base/util.h"
#include "qslary/net/EventLoop.h"
#include <bits/types/struct_itimerspec.h>
#include <bits/types/struct_timespec.h>
#include <cstdint>
#include <ctime>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>

namespace qslary
{
namespace detail
{

int createTimerfd()
{
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

  if (timerfd < 0)
  {
    assert(false);
  }
  return timerfd;
}

struct timespec howMuchTimeFromNow(Timestamp when)
{
  int64_t microseconds = when.microSecondsSinceEpoch() - Timestamp::now().microSecondsSinceEpoch();
  if (microseconds < 100)
  {
    microseconds = 100;
  }

  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(microseconds / Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<long>((microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
  return ts;
}


void readTimerfd(int timerfd, Timestamp now)
{
  uint64_t howmany;
  ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
  // TAG logging
  if (n != sizeof howmany)
    assert(false);
}

void resetTimerfd(int timerfd, Timestamp expiration)
{
  struct itimerspec newvalue;
  struct itimerspec oldvalue;
  memZero(&newvalue, sizeof newvalue);
  memZero(&oldvalue, sizeof newvalue);
  newvalue.it_value = howMuchTimeFromNow(expiration);
  int ret = ::timerfd_settime(timerfd, 0, &newvalue, &oldvalue);
  if (ret)
  {
    assert(false);
  }
}

} // namespace detail
} // namespace qslary



namespace qslary
{

static Logger::ptr g_logger=QSLARY_LOG_NAME("system");


void Timer::restart(Timestamp now)
{
  if (recurring_)
  {
    expiration_ = addTime(now, interval_);
  }
  else
  {
    expiration_ = Timestamp::invalid();
  }
}

TimerManger::TimerManger(EventLoop* loop)
    : loop_(loop), timerfd_(detail::createTimerfd()), timerfdChannel_(loop, timerfd_), timers_(), callingExpiredTimers_(false)
{
  timerfdChannel_.setReadCallback(std::bind(&TimerManger::handleRead, this));
  // we are always reading the timerfd, we disarm it with timerfd_settime.
  timerfdChannel_.enableReading();
}

TimerManger::~TimerManger()
{
  timerfdChannel_.disableAll();
  timerfdChannel_.remove();
  ::close(timerfd_);
  // do not remove channel, since we're in EventLoop::dtor();
  for (const Entry& timer : timers_)
  {
    delete timer.second;
  }
}

TimerId TimerManger::addTimer(std::function<void()> cb, Timestamp when, double interval)
{
  Timer* timer = new Timer(std::move(cb), when, interval);
  loop_->runInLoop(std::bind(&TimerManger::addTimerInLoop, this, timer));
  return TimerId(timer, timer->sequence());
}

void TimerManger::cancel(TimerId timerId) { loop_->runInLoop(std::bind(&TimerManger::cancelInLoop, this, timerId)); }

void TimerManger::addTimerInLoop(Timer* timer)
{
  loop_->assertInLoopThread();
  bool earliestChanged = insert(timer);

  if (earliestChanged)
  {
    detail::resetTimerfd(timerfd_, timer->expiration());
  }
}

void TimerManger::cancelInLoop(TimerId timerId)
{
  loop_->assertInLoopThread();
  assert(timers_.size() == activeTimers_.size());
  ActiveTimer timer(timerId.timer_, timerId.sequence_);
  ActiveTimerSet::iterator it = activeTimers_.find(timer);
  if (it != activeTimers_.end())
  {
    size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
    assert(n == 1);
    (void)n;
    delete it->first; // FIXME: no delete please
    activeTimers_.erase(it);
  }
  else if (callingExpiredTimers_)
  {
    cancelingTimers_.insert(timer);
  }
  assert(timers_.size() == activeTimers_.size());
}

void TimerManger::handleRead()
{
  loop_->assertInLoopThread();
  Timestamp now(Timestamp::now());
  detail::readTimerfd(timerfd_, now);

  std::vector<Entry> expired = getExpired(now);

  callingExpiredTimers_ = true;
  cancelingTimers_.clear();
  // safe to callback outside critical section
  for (const Entry& it : expired)
  {
    it.second->run();
  }
  callingExpiredTimers_ = false;

  reset(expired, now);
}

// getExpired 之后会删除 activeTimers中与timers_中的定时器
std::vector<TimerManger::Entry> TimerManger::getExpired(Timestamp now)
{
  assert(timers_.size() == activeTimers_.size());
  std::vector<Entry> expired;
  Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
  TimerList::iterator end = timers_.lower_bound(sentry);
  assert(end == timers_.end() || now < end->first);
  std::copy(timers_.begin(), end, back_inserter(expired));
  timers_.erase(timers_.begin(), end);

  for (const Entry& it : expired)
  {
    ActiveTimer timer(it.second, it.second->sequence());
    size_t n = activeTimers_.erase(timer);
    assert(n == 1);
    (void)n;
  }

  assert(timers_.size() == activeTimers_.size());
  return expired;
}

void TimerManger::reset(const std::vector<Entry>& expired, Timestamp now)
{
  Timestamp nextExpire;

  for (const Entry& it : expired)
  {
    ActiveTimer timer(it.second, it.second->sequence());
    if (it.second->repate() && cancelingTimers_.find(timer) == cancelingTimers_.end())
    {
      it.second->restart(now);
      insert(it.second);
    }
    else
    {
      // FIXME move to a free list
      delete it.second; // FIXME: no delete please
    }
  }

  if (!timers_.empty())
  {
    nextExpire = timers_.begin()->second->expiration();
  }

  if (nextExpire.valid())
  {
    detail::resetTimerfd(timerfd_, nextExpire);
  }
}

bool TimerManger::insert(Timer* timer)
{
  loop_->assertInLoopThread();
  assert(timers_.size() == activeTimers_.size());
  bool earliestChanged = false;
  Timestamp when = timer->expiration();
  TimerList::iterator it = timers_.begin();
  if (it == timers_.end() || when < it->first)
  {
    earliestChanged = true;
  }
  {
    std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
    assert(result.second);
    (void)result;
  }
  {
    std::pair<ActiveTimerSet::iterator, bool> result = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
    assert(result.second);
    (void)result;
  }

  assert(timers_.size() == activeTimers_.size());
  return earliestChanged;
}

} // namespace qslary
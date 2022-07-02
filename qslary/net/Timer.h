#ifndef __QSLARY_TIMER_H_
#define __QSLARY_TIMER_H_

#include "qslary/base/Atomic.h"
#include "qslary/base/Thread.h"
#include "qslary/net/Channel.h"
#include "qslary/net/TimerId.h"
#include "qslary/net/Timestamp.h"

#include <cstdint>
#include <memory>
#include <set>
#include <vector>

namespace qslary
{

class TimerManger;
class Timer : public std::enable_shared_from_this<Timer>
{
  friend class TimerManger;

public:
  typedef std::shared_ptr<Timer> ptr;

  Timer(std::function<void()> cb, Timestamp when, double interval)
      : cb_(cb), expiration_(when), interval_(interval), recurring_(interval > 0.0),
        sequence_(s_numCreated_.incrementAndGet())
  {
  }
  
  

  void run() const
  {
    cb_();
  }

  Timestamp expiration() const { return expiration_; }
  bool repate() const { return recurring_; }
  int64_t sequence() const { return sequence_; }

  void restart(Timestamp now);

  static int64_t numCreated() { return s_numCreated_.get(); }


private:
  
  std::function<void()> cb_;
  Timestamp expiration_;
  const double interval_;
  const bool recurring_;
  const int64_t sequence_;
  static AtomicInt64 s_numCreated_;
};









class EventLoop;

class TimerManger
{
  friend class Timer;

public:
  typedef std::shared_ptr<TimerManger> ptr;

public:
  explicit TimerManger(EventLoop* loop);
  ~TimerManger();

  ///
  /// Schedules the callback to be run at given time,
  /// repeats if @c interval > 0.0.
  ///
  /// Must be thread safe. Usually be called from other threads.
  TimerId addTimer(std::function<void()> cb, Timestamp when, double interval);

  void cancel(TimerId timerId);

private:
  // FIXME: use unique_ptr<Timer> instead of raw pointers.
  // This requires heterogeneous comparison lookup (N3465) from C++14
  // so that we can find an T* in a set<unique_ptr<T>>.
  typedef std::pair<Timestamp, Timer*> Entry;
  typedef std::set<Entry> TimerList;
  typedef std::pair<Timer*, int64_t> ActiveTimer;
  typedef std::set<ActiveTimer> ActiveTimerSet;

  void addTimerInLoop(Timer* timer);
  void cancelInLoop(TimerId timerId);
  // called when timerfd alarms
  void handleRead();
  // move out all expired timers
  std::vector<Entry> getExpired(Timestamp now);
  void reset(const std::vector<Entry>& expired, Timestamp now);

  bool insert(Timer* timer);

  EventLoop* loop_;
  const int timerfd_;
  Channel timerfdChannel_;
  // Timer list sorted by expiration
  TimerList timers_;

  // for cancel()
  ActiveTimerSet activeTimers_;
  bool callingExpiredTimers_; /* atomic */
  ActiveTimerSet cancelingTimers_;
};
} // namespace qslary

#endif
#ifndef __QSLARY_TIMER_H_
#define __QSLARY_TIMER_H_
#include <cstdint>
#include <memory>
#include <set>
#include <vector>
#include "qslary/base/Atomic.h"
#include "qslary/base/Thread.h"
#include "qslary/net/TimerId.h"
#include "qslary/net/Timestamp.h"

namespace qslary
{

class TimerManger;
class Timer : public std::enable_shared_from_this<Timer>
{

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

  void restart(Timestamp now);

  Timestamp expiration() const { return expiration_; }
  bool repate() const { return recurring_; }
  int64_t sequence() const { return sequence_; }

  static int64_t numCreated() { return s_numCreated_.get(); }

private:
  std::function<void()> cb_;
  Timestamp expiration_;
  const double interval_;
  const bool recurring_;
  const int64_t sequence_;
  static AtomicInt64 s_numCreated_;

friend class TimerManger;
};

class TimerManger
{

public:
  typedef std::shared_ptr<TimerManger> ptr;

public:
  TimerManger();
  ~TimerManger();

  TimerId addTimer(std::function<void()> cb, Timestamp when, double interval);
  
  TimerId runAt(Timestamp time, std::function<void()> cb);
  TimerId runAfter(double delay, std::function<void()> cb);
  TimerId runEvery(double interval, std::function<void()> cb);

  void cancel(TimerId timerId);

protected:
  typedef std::pair<Timestamp, Timer*> Entry;
  typedef std::set<Entry> TmierSet;
  typedef std::pair<Timer*, int64_t> ActiveTimer;
  typedef std::set<ActiveTimer> ActiveTimerSet;

  void addTimer(Timer* timer);
  void handleRead();
  std::vector<Entry> getExpired(Timestamp now);
  void reset(const std::vector<Entry>& expired, Timestamp now);

  bool insert(Timer* timer);

  int timerfd_;
  TmierSet timers_;

  // for cancel()
  ActiveTimerSet activeTimers_;
  bool callingExpiredTimers_; 
  ActiveTimerSet cancelingTimers_;
friend class Timer;
};
} // namespace qslary

#endif
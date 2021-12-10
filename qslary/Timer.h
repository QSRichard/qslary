#ifndef __QSLARY_TIMER_H_
#define __QSLARY_TIMER_H_

#include "thread.h"

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
  bool cancel();
  bool refersh();
  bool reset(uint64_t ms, bool from_now);

private:
  Timer(std::uint64_t ms, std::function<void()> cb, bool recurring, TimerManger* TimerManger);
  Timer(uint64_t next);

private:
  std::uint64_t ms_ = 0;
  std::function<void()> cb_;
  bool recurring_ = false;
  std::uint64_t next_ = 0;
  TimerManger* manager_ = nullptr;
  struct Comparator
  {
    bool operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const;
  };
};

class TimerManger
{
  friend class Timer;

public:
  TimerManger();
  virtual ~TimerManger();

  Timer::ptr addTimer(std::uint64_t ms, std::function<void()> cb, bool recurring = false);
  Timer::ptr addConditionTimer(std::uint64_t ms, std::function<void()> cb, std::weak_ptr<void> weak_cond,
                               bool recurring = false);
  uint64_t getNextTimer();
  void listExpiredCallerBack(std::vector<std::function<void()>>& callbacks);

protected:
  virtual void onTimerInsertAtFront() = 0;

private:
  // 检测服务器时间是否发生修改
  bool detectClockRollover(uint64_t now_ms);

private:
  qslary::RWMutex mutex_;
  std::set<Timer::ptr, Timer::Comparator> timers_;
  uint64_t previousTime;
};
} // namespace qslary

#endif
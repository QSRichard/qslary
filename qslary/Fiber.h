#ifndef __QSLARY_FIBER_H_
#define __QSLARY_FIBER_H_

#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>

#include <functional>
#include <memory>

#include "mutex.h"

namespace qslary {
class Scheduler;

class Fiber : public std::enable_shared_from_this<Fiber> {
  friend class Scheduler;

 public:
  typedef std::shared_ptr<Fiber> ptr;
  enum State { INIT, HOLD, EXEC, TERM, READY, EXCEPTION };

 private:
  Fiber();

 public:
  explicit Fiber(std::function<void()> cb, size_t stacksize = 0,
                 bool ScheduleruseCaller = false);
  ~Fiber();

  /**
   * @brief 
   * @pre  
   *@post  
   */
  void swapIn();

  /**
   * @brief 
   * @pre   
   * @post  
   *
   */
  void swapOut();

  /**
   * @brief 将线程从主协程切换为this Fiber
   * @pre   Main Fiber
   * @post this Fiber
   */
  void call();

  /**
   * @brief 将线程从this Fiber切换为主协程
   * @pre   this Fiber
   * @post  Main Fiber
   *
   */
  void back();
  
  /**
   * @brief 重置协程任务函数，并设置状态
   * @pre getState() 为 INIT，TERM，EXCEPTION
   * @post getState() INIT
   * @param cb 任务函数
   */
  void reset(std::function<void()> cb);

  qslary::Fiber::State getState() { return state_; }

  uint64_t getId() const { return id_; }

 public:
  static void SetThreadCurrentFiber(Fiber* f);
  static Fiber::ptr GetThreadCurrentFiber();
  // 协程切换到后台，并且设置为Ready状态
  static void YieldToReady();
  // 协程切换到后台，并且设置Hold状态
  static void YieldToHold();
  static uint64_t TotalFibers();

  static uint64_t getFiberId();

  static void MainFunc();
  static void CallMainFunc();

 private:
  uint64_t id_ = 0;
  uint32_t stacksize_ = 0;
  State state_ = INIT;
  bool useCaller = false;
  ucontext_t ctx_;
  void* stack_ = nullptr;

  std::function<void()> cb_;
};
}  // namespace qslary

#endif  // __QSLARY_FIBER_H_
#ifndef __QSLARY_FIBER_H_
#define __QSLARY_FIBER_H_

#include "mutex.h"

#include <ucontext.h>
#include <unistd.h>
#include <stdio.h>
#include <memory>
#include <functional>

namespace qslary
{

  class Fiber : public std::enable_shared_from_this<Fiber>
  {
  public:
    typedef std::shared_ptr<Fiber> ptr;
    enum State
    {
      INIT,
      HOLD,
      EXEC,
      TERM,
      READY,
      EXCEPTION
    };

  private:
    Fiber();

  public:
    explicit Fiber(std::function<void()> cb, size_t stacksize = 0);
    ~Fiber();
    // 切换到当前协程执行
    void swapIn();
    // 切换到后台执行
    void swapOut();
    // 处于INIT或TERM状态时 重置协程函数 并重置状态
    void reset(std::function<void()> cb);

    uint64_t getId() const { return id_; }

  public:
    static void SetThis(Fiber *f);
    static Fiber::ptr GetThis();
    // 协程切换到后台，并且设置为Ready状态
    static void YieldToReady();
    // 协程切换到后台，并且设置Hold状态
    static void YieldToHold();
    static uint64_t TotalFibers();

    static uint64_t getFiberId();

    static void MainFunc();

  private:
    uint64_t id_ = 0;
    uint32_t stacksize_ = 0;
    State state_ = INIT;

    ucontext_t ctx_;
    void *stack_ = nullptr;

    std::function<void()> cb_;
  };
} // namespace qslary end

#endif // __QSLARY_FIBER_H_
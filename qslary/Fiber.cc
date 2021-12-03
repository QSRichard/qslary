#include "config.h"
#include "Fiber.h"
#include "logger.h"

#include <atomic>
#include <iostream>

namespace qslary
{

  static std::atomic<uint32_t> s_fiber_count(0);
  static std::atomic<uint64_t> s_fiber_id(0);

  static thread_local qslary::Fiber *t_fiber = nullptr;
  static thread_local qslary::Fiber::ptr t_threadFiber = nullptr;

  qslary::Logger::ptr g_logger=QSLARY_LOG_NAME("system");

  // TODO int32_t  --->  uint32_t
  static ConfigVar<int32_t>::ptr g_fiber_stack_size =
      Config::lookup("fiber.static_size", 1024 * 1024, "fiber static size");

  class MallocStackAllocator
  {
  public:
    static void *Alloc(size_t size)
    {
      return malloc(size);
    }

    static void Dealloc(void *vp, size_t size)
    {
      return free(vp);
    }
  };

  using StackAllocator = MallocStackAllocator;

  uint64_t Fiber::getFiberId()
  {
    if (t_fiber)
    {
      return t_fiber->getId();
    }
    return 0;
  }

  void Fiber::SetThis(Fiber *f)
  {
    t_fiber = f;
  }

  Fiber::ptr Fiber::GetThis()
  {
    if (t_fiber)
    {
      return t_fiber->shared_from_this();
    }

    Fiber::ptr main_fiber(new Fiber);
    assert(t_fiber == main_fiber.get());

    t_threadFiber = main_fiber;
    return t_fiber->shared_from_this();
  }
  Fiber::Fiber()
  {
    state_ = EXEC;
    SetThis(this);
    if (getcontext(&ctx_))
    {
      // TODO error handling
      QSLARY_LOG_ERROR(g_logger)<<"getcontext error";
    }
    ++s_fiber_count;
    // QSLARY_LOG_DEBUG(g_logger)<<"Fiber() Construct Complete id "<<id_;
  }

  Fiber::Fiber(std::function<void()> cb, size_t stacksize) : id_(++s_fiber_id),
                                                             cb_(cb)
  {
    ++s_fiber_count;
    stacksize_ = stacksize ? stacksize : g_fiber_stack_size->getValue();
    stack_ = StackAllocator::Alloc(stacksize_);
    if (getcontext(&ctx_))
    {
      // TODO error handling
      QSLARY_LOG_ERROR(g_logger)<<"getcontext error";
    }
    ctx_.uc_link = nullptr;
    ctx_.uc_stack.ss_sp = stack_;
    ctx_.uc_stack.ss_size = stacksize_;
    makecontext(&ctx_, &Fiber::MainFunc, 0);
    // QSLARY_LOG_DEBUG(g_logger)<<"Fiber(cb,stacksize) Construct Complete id "<<id_;
  }

  Fiber::~Fiber()
  {
    --s_fiber_count;
    if (stack_)
    {
      // TODO QSALRY_ASSERT
      assert(state_ == State::TERM || state_ == State::INIT);

      StackAllocator::Dealloc(stack_, stacksize_);
    }
    else
    {
      // TODO QSLARY_ASSERT
      assert(!cb_);
      assert(state_ == State::EXEC);
      Fiber *cur = t_fiber;
      if (cur == this)
      {
        SetThis(nullptr);
      }
    }
    // QSLARY_LOG_DEBUG(g_logger) << "~Fiber() id " << id_;
  }

  // 切换到当前协程执行
  void Fiber::swapIn()
  {
    SetThis(this);

    // assert(state_==State::EXEC);

    state_ = State::EXEC;

    ucontext_t *p1 = &(t_threadFiber->ctx_);
    ucontext_t *p2 = &ctx_;

    if (swapcontext(p1, p2))
    {
      // TODO handling swapcontex error
      QSLARY_LOG_ERROR(g_logger)<<"swapcontext error";
    }
  }
  // 切换到后台执行
  void Fiber::swapOut()
  {

    SetThis(t_threadFiber.get());
    if (swapcontext(&ctx_, &(t_threadFiber->ctx_)))
    {
      // TODO handling swapcontext error
      QSLARY_LOG_ERROR(g_logger)<<"swapcontext error";
    }
  }

  // 处于INIT或TERM状态时 重置协程函数 并重置状态
  void Fiber::reset(std::function<void()> cb)
  {

    // TODO QSLARY_ASSERT state_ && stack_

    cb_ = cb;
    if (getcontext(&ctx_))
    {
      // TODO error handling
      QSLARY_LOG_ERROR(g_logger)<<"getcontext error";
    }
    ctx_.uc_link = nullptr;
    ctx_.uc_stack.ss_sp = stack_;
    ctx_.uc_stack.ss_size = stacksize_;
    makecontext(&ctx_, &Fiber::MainFunc, 0);
    state_ = INIT;
  }

  // 协程切换到后台，并且设置为Ready状态
  void Fiber::YieldToReady()
  {
    Fiber::ptr cur = GetThis();
    cur->state_ = State::READY;
    cur->swapOut();
  }
  // 协程切换到后台，并且设置Hold状态
  void Fiber::YieldToHold()
  {
    Fiber::ptr cur = GetThis();
    cur->state_ = HOLD;
    cur->swapOut();
  }

  void Fiber::MainFunc()
  {

    Fiber::ptr cur = GetThis();
    assert(cur);
    try
    {
      cur->cb_();
      cur->cb_ = nullptr;
      cur->state_ = State::TERM;
    }
    catch (std::exception &ex)
    {
      cur->state_ = State::EXCEPTION;
      // TODO logging exception
      QSLARY_LOG_ERROR(g_logger)<<"Fiber::MainFunc exception";

    }
    catch (...)
    {
      cur->state_ = State::EXCEPTION;
      // TODO logging exception
      QSLARY_LOG_ERROR(g_logger)<<"Fiber::MainFunc exception";
    }

    auto raw_ptr=cur.get();
    cur.reset();
    raw_ptr->swapOut();

    // cur->swapOut();
    // std::cout<<"cur swapout ";

  }

  uint64_t Fiber::TotalFibers()
  {
    return s_fiber_count;
  }

} // namespace qslary end
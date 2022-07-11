#include "qslary/base/Fiber.h"
#include "qslary/base/Config.h"
#include "qslary/base/CurrentThread.h"
#include "qslary/base/Logger.h"
#include "qslary/net/Scheduler.h"

#include <atomic>
#include <iostream>

namespace qslary {

static std::atomic<uint32_t> s_fiber_count(0);
static std::atomic<uint64_t> s_fiber_id(0);

static thread_local qslary::Fiber* thread_current_fiber = nullptr;
static thread_local qslary::Fiber::ptr thread_main_fiber = nullptr;

qslary::Logger::ptr g_logger = QSLARY_LOG_NAME("system");

// TODO int32_t  --->  uint32_t
static ConfigVar<int32_t>::ptr g_fiber_stack_size =
    Config::lookup("fiber.static_size", 1024 * 1024, "fiber static size");

class MallocStackAllocator {
 public:
  static void* Alloc(size_t size) { return malloc(size); }

  static void Dealloc(void* vp, size_t size) { return free(vp); }
};

using StackAllocator = MallocStackAllocator;

uint64_t Fiber::getFiberId() {
  if (thread_current_fiber) {
    return thread_current_fiber->getId();
  }
  // FIXME 0 是否合法
  return 0;
}

/**
 * @brief 为线程设置当前协程 内部使用
 *
 * @param f
 */
void Fiber::SetThreadCurrentFiber(Fiber* f) { thread_current_fiber = f; }

/**
 * @brief 首次调用将设置线程的主协程 在一个线程中使用协程时需要首先调用这个函数
 *
 * @return Fiber::ptr
 */
Fiber::ptr Fiber::GetThreadCurrentFiber() {
  if (thread_current_fiber) 
  {
    return thread_current_fiber->shared_from_this();
  }
  Fiber::ptr main_fiber(new Fiber);
  // FIXME assert 放在这里可以吗
  assert(thread_current_fiber == main_fiber.get());
  thread_main_fiber = main_fiber;
  return thread_current_fiber->shared_from_this();
}

// 每次创建Fiber时都会将当前Fiber置为线程当前Fiber
Fiber::Fiber() 
{
  state_ = State::EXEC;
  SetThreadCurrentFiber(this);
  if (getcontext(&ctx_)) 
  {
    QSLARY_LOG_ERROR(g_logger) << "getcontext error";
  }
  id_=++s_fiber_id;
  s_fiber_count++;
  QSLARY_LOG_ERROR(g_logger) << "Fiber::Fiber main";
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize)
    : id_(++s_fiber_id), cb_(cb)
{
  ++s_fiber_count;
  stacksize_ = stacksize ? stacksize : g_fiber_stack_size->getValue();
  stack_ = StackAllocator::Alloc(stacksize_);
  if (getcontext(&ctx_)) 
  {
    QSLARY_LOG_ERROR(g_logger) << "getcontext error";
  }
  ctx_.uc_link = nullptr;
  ctx_.uc_stack.ss_sp = stack_;
  ctx_.uc_stack.ss_size = stacksize_;
  makecontext(&ctx_, &Fiber::FiberRun, 0);
}

Fiber::~Fiber() 
{
  --s_fiber_count;
  if (stack_) 
  {
    assert(state_ == State::TERM || state_ == State::INIT ||
           state_ == State::EXCEPTION);

    StackAllocator::Dealloc(stack_, stacksize_);
  } 
  else 
  {
    assert(!cb_);
    assert(state_ == State::EXEC);
    Fiber* cur = thread_current_fiber;
    if (cur == this) 
    {
      SetThreadCurrentFiber(nullptr);
    }
  }
}

// 切换到当前协程执行
void Fiber::swapIn() 
{
  SetThreadCurrentFiber(this);
  assert(state_ != State::EXEC);
  state_ = State::EXEC;
  if (swapcontext(&(Scheduler::GetSchedulerFirstFiber()->ctx_), &ctx_)) 
  {
    QSLARY_LOG_ERROR(g_logger) << "swapcontext error";
  }
}

// 切换到后台执行
// FIXME 状态不用改变吗
void Fiber::swapOut()
{

  SetThreadCurrentFiber(Scheduler::GetSchedulerFirstFiber());
  if (swapcontext(&ctx_, &(Scheduler::GetSchedulerFirstFiber()->ctx_))) 
  {
    QSLARY_LOG_ERROR(g_logger) << "swapcontext error";
  }
}

// 处于INIT或TERM或EXCEPTION状态时 重置协程函数 并重置状态
void Fiber::reset(std::function<void()> cb) 
{
  assert(stack_);
  assert(state_ == State::TERM || state_ == State::EXCEPTION ||
         state_ == State::INIT);

  cb_ = cb;
  if (getcontext(&ctx_)) 
  {
    QSLARY_LOG_ERROR(g_logger) << "getcontext error";
  }
  // FIXME 这里ctx.uc_link 设置为nullptr 没有问题吗
  ctx_.uc_link = nullptr;
  ctx_.uc_stack.ss_sp = stack_;
  ctx_.uc_stack.ss_size = stacksize_;
  makecontext(&ctx_, &Fiber::FiberRun, 0);
  state_ = State::INIT;
}

// 协程切换到后台，并且设置为Ready状态
void Fiber::YieldToReady() 
{
  Fiber::ptr cur = GetThreadCurrentFiber();
  assert(cur->state_ == State::EXEC);
  cur->state_ = State::READY;
  cur->swapOut();
}

// 协程切换到后台，并且设置Hold状态
void Fiber::YieldToHold() 
{
  Fiber::ptr cur = GetThreadCurrentFiber();
  assert(cur->state_ == State::EXEC);
  cur->state_ = HOLD;
  cur->swapOut();
}

void Fiber::FiberRun()
{

  Fiber::ptr fiber = GetThreadCurrentFiber();
  assert(fiber);
  try 
  {
    fiber->cb_();
    fiber->cb_ = nullptr;
    fiber->state_ = State::TERM;
  } 
  catch (std::exception& ex) 
  {
    fiber->state_ = State::EXCEPTION;
    std::cout << qslary::CurrentThread::stackTrace(true)<<std::endl;
    QSLARY_LOG_ERROR(g_logger) << "Fiber::MainFunc exception";
  } 
  catch (...) 
  {
    fiber->state_ = State::EXCEPTION;
    QSLARY_LOG_ERROR(g_logger) << "Fiber::MainFunc exception";
  }

  auto raw_ptr = fiber.get();
  fiber.reset();
  raw_ptr->swapOut();
  std::cout << " never occur reach" << std::endl;
}

uint64_t Fiber::TotalFibers()
{
  return s_fiber_count;
}

}  // namespace qslary
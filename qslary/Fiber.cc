#include "Fiber.h"

#include <atomic>
#include <iostream>

#include "Scheduler.h"
#include "config.h"
#include "logger.h"

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
  if (thread_current_fiber) {
    return thread_current_fiber->shared_from_this();
  }
  Fiber::ptr main_fiber(new Fiber);
  assert(thread_current_fiber == main_fiber.get());
  thread_main_fiber = main_fiber;
  return thread_current_fiber->shared_from_this();
}

Fiber::Fiber() {
  state_ = State::EXEC;
  SetThreadCurrentFiber(this);
  if (getcontext(&ctx_)) {
    QSLARY_LOG_ERROR(g_logger) << "getcontext error";
  }
  id_=++s_fiber_id;
  s_fiber_count++;
  QSLARY_LOG_DEBUG(g_logger) << "Fiber() Construct Complete id " << id_;
}

Fiber::Fiber(std::function<void()> cb, size_t stacksize,
             bool ScheduleruseCaller)
    : id_(++s_fiber_id), useCaller(ScheduleruseCaller),cb_(cb){
  ++s_fiber_count;
  stacksize_ = stacksize ? stacksize : g_fiber_stack_size->getValue();
  stack_ = StackAllocator::Alloc(stacksize_);
  if (getcontext(&ctx_)) {
    QSLARY_LOG_ERROR(g_logger) << "getcontext error";
  }
  ctx_.uc_link = nullptr;
  ctx_.uc_stack.ss_sp = stack_;
  ctx_.uc_stack.ss_size = stacksize_;
  if (!useCaller) {
    makecontext(&ctx_, &Fiber::MainFunc, 0);
  } else {
    makecontext(&ctx_, &Fiber::CallMainFunc, 0);
  }
  QSLARY_LOG_DEBUG(g_logger)
      << "Fiber(cb,stacksize) Construct Complete id" << id_;
}

Fiber::~Fiber() {
  --s_fiber_count;
  if (stack_) {
    assert(state_ == State::TERM || state_ == State::INIT ||
           state_ == State::EXCEPTION);

    StackAllocator::Dealloc(stack_, stacksize_);
  } else {
    assert(!cb_);
    assert(state_ == State::EXEC);
    Fiber* cur = thread_current_fiber;
    if (cur == this) {
      SetThreadCurrentFiber(nullptr);
    }
  }
  QSLARY_LOG_DEBUG(g_logger) << "~Fiber() id " << id_;
}

// 切换到当前协程执行
void Fiber::swapIn() {
  SetThreadCurrentFiber(this);
  assert(state_ != State::EXEC);
  state_ = State::EXEC;
  if (swapcontext(&(Scheduler::GetMainFiber()->ctx_), &ctx_)) {
    QSLARY_LOG_ERROR(g_logger) << "swapcontext error";
  }
}

// 切换到后台执行
// FIXME 状态不用改变吗
void Fiber::swapOut() {
  
  QSLARY_LOG_DEBUG(g_logger)<<"entry swapOut";
  SetThreadCurrentFiber(Scheduler::GetMainFiber());
  if (swapcontext(&ctx_, &(Scheduler::GetMainFiber()->ctx_))) {
    QSLARY_LOG_ERROR(g_logger) << "swapcontext error";
  }
}

void Fiber::call() {

  SetThreadCurrentFiber(this);
  state_ = State::EXEC;
  ucontext_t* p1 = &(thread_main_fiber->ctx_);
  ucontext_t* p2 = &ctx_;
  if (swapcontext(p1, p2)) {
    QSLARY_LOG_ERROR(g_logger) << "swapcontext error";
  }
}

void Fiber::back() {
  SetThreadCurrentFiber(thread_main_fiber.get());
  if (swapcontext(&ctx_, &(thread_main_fiber->ctx_))) {
    QSLARY_LOG_ERROR(g_logger) << "swapcontext error";
  }
}

// 处于INIT或TERM或EXCEPTION状态时 重置协程函数 并重置状态
void Fiber::reset(std::function<void()> cb) {
  assert(stack_);
  assert(state_ == State::TERM || state_ == State::EXCEPTION ||
         state_ == State::INIT);

  cb_ = cb;
  if (getcontext(&ctx_)) {
    QSLARY_LOG_ERROR(g_logger) << "getcontext error";
  }
  ctx_.uc_link = nullptr;
  ctx_.uc_stack.ss_sp = stack_;
  ctx_.uc_stack.ss_size = stacksize_;
  makecontext(&ctx_, &Fiber::MainFunc, 0);
  state_ = State::INIT;
}

// 协程切换到后台，并且设置为Ready状态
void Fiber::YieldToReady() {
  Fiber::ptr cur = GetThreadCurrentFiber();
  assert(cur->state_ == State::EXEC);
  cur->state_ = State::READY;
  cur->swapOut();
}

// 协程切换到后台，并且设置Hold状态
void Fiber::YieldToHold() {
  Fiber::ptr cur = GetThreadCurrentFiber();
  assert(cur->state_ == State::EXEC);
  cur->state_ = HOLD;
  cur->swapOut();
}

/**
 * @brief 协程的工作函数 任务将在这个函数中调用 任务执行完成后 将返回主协程
 *        MainFunc执行完任务函数之后调用 swapOut切换回线程中的Scheduler的协程
 *
 */
void Fiber::MainFunc() {
  Fiber::ptr cur = GetThreadCurrentFiber();
  assert(cur);
  try {
    cur->cb_();
    cur->cb_ = nullptr;
    cur->state_ = State::TERM;
  } catch (std::exception& ex) {
    cur->state_ = State::EXCEPTION;
    QSLARY_LOG_ERROR(g_logger) << "Fiber::MainFunc exception";

  } catch (...) {
    cur->state_ = State::EXCEPTION;
    QSLARY_LOG_ERROR(g_logger) << "Fiber::MainFunc exception";
  }

  auto raw_ptr = cur.get();
  cur.reset();
  raw_ptr->swapOut();
  std::cout << " never occur reach" << std::endl;
}

void Fiber::CallMainFunc() {
  Fiber::ptr cur = GetThreadCurrentFiber();
  assert(cur);
  try {
    cur->cb_();
    cur->cb_ = nullptr;
    cur->state_ = State::TERM;
  } catch (std::exception& ex) {
    cur->state_ = State::EXCEPTION;
    QSLARY_LOG_ERROR(g_logger) << "Fiber::MainFunc exception";

  } catch (...) {
    cur->state_ = State::EXCEPTION;
    QSLARY_LOG_ERROR(g_logger) << "Fiber::MainFunc exception";
  }

  auto raw_ptr = cur.get();
  cur.reset();
  raw_ptr->back();
  std::cout << "never reach" << std::endl;
}

uint64_t Fiber::TotalFibers() { return s_fiber_count; }

}  // namespace qslary
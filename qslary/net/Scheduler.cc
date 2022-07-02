#include "qslary/net/Scheduler.h"
#include "qslary/base/Fiber.h"
#include "qslary/base/Hook.h"
#include "qslary/base/Logger.h"

#include <bits/c++config.h>
#include <cstdint>
#include <string>

namespace qslary
{
// extern uint64_t s_fiber_id;

static qslary::Logger::ptr g_logger = QSLARY_LOG_NAME("system");

static thread_local Scheduler* thread_scheduler = nullptr;
static thread_local Fiber* thread_scheduler_first_fiber = nullptr;

/**
 * @brief
 *
 * @return Scheduler*
 */
Scheduler* Scheduler::GetThreadScheduler()
{
  return thread_scheduler;
}

/**
 * @brief
 *
 * @return Fiber*
 */

Fiber* Scheduler::GetSchedulerFirstFiber()
{
  return thread_scheduler_first_fiber;
}

void Scheduler::setThreadScheduler()
{
  thread_scheduler = this;
}

Scheduler::Scheduler(std::size_t threadNum, const std::string& name)
{
  assert(threadNum > 0);
  name_ = name;
  threadCount_ = threadNum;
  activeThreadCount_ = threadNum;
}

Scheduler::~Scheduler()
{

  assert(stopping_);
  if (GetThreadScheduler() == this)
  {
    thread_scheduler = nullptr;
  }
}

void Scheduler::start()
{
  qslary::MutexLockGuard lock(mutex_);
  if (!stopping_)
  {
    return;
  }
  stopping_ = false;
  assert(threads_.empty());
  threads_.resize(threadCount_);
  for (size_t i = 0; i < threadCount_; i++)
  {
    // TODO thread name
    
    threads_[i].reset(new qslary::Thread(std::bind(&Scheduler::run, this),"Scheduler::Thread "+std::to_string(i)));
    threads_[i]->start();
    threadsId_.push_back(threads_[i]->getId());
  }
}

void Scheduler::stop()
{
  autoStop_ = true;
  if ( threadCount_ == 0 )
  {
    QSLARY_LOG_INFO(g_logger) << "stopped" << this;
    stopping_ = true;
    if (stopping())
    {
      return;
    }
  }
  stopping_=true;

  assert(GetThreadScheduler() != this);


  for (auto& i : threads_)
  {
    i->join();
  }
}

void Scheduler::run()
{
  setThreadScheduler();

  set_hook_enable(true);
  
  // 新线程 为其创建主协程
  thread_scheduler_first_fiber = qslary::Fiber::GetThreadCurrentFiber().get();
}

void Scheduler::tickle()
{
  QSLARY_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping()
{
  qslary::MutexLockGuard lock(mutex_);
  return autoStop_ && stopping_ && pendingFunctors_.empty() && activeThreadCount_ == 0;
}

void Scheduler::idle()
{
  QSLARY_LOG_INFO(g_logger) << "idle";
  while (!stopping())
  {
    QSLARY_LOG_INFO(g_logger) << "idle";
    qslary::Fiber::YieldToHold();
  }
}

} // namespace qslary
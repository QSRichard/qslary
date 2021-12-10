#include "Scheduler.h"

#include <bits/c++config.h>

#include "Fiber.h"
#include "logger.h"

namespace qslary
{

static qslary::Logger::ptr g_logger = QSLARY_LOG_NAME("system");

static thread_local Scheduler* thread_scheduler = nullptr;
static thread_local Fiber* thread_scheduler_fiber = nullptr;

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

Fiber* Scheduler::GetMainFiber()
{
  return thread_scheduler_fiber;
}

void Scheduler::setThreadScheduler()
{
  thread_scheduler = this;
}

Scheduler::Scheduler(std::size_t threadNum, bool useCaller, const std::string& name)
{
  QSLARY_LOG_DEBUG(g_logger)<<"Scheduler()构造函数";
  assert(threadNum > 0);
  if (useCaller)
  {
    // 首次调用 为线程设置主协程
    qslary::Fiber::GetThreadCurrentFiber();
    --threadNum;
    assert(thread_scheduler == nullptr);
    thread_scheduler=this;
    rootFiber_.reset(new Fiber(std::bind(&Scheduler::run, this), 0, true));
    thread_scheduler_fiber = rootFiber_.get();
    rootThreadId = qslary::CurrentThread::tid();
    threadsId_.push_back(rootThreadId);
  }
  else
  {
    rootThreadId = -1;
  }
  name_ = name;
  useCaller_ = useCaller;
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
  QSLARY_LOG_DEBUG(g_logger)
      << "entry Scheduler::start() 将在这里创建线程并且绑定线程的函数为Scheduler::run()，并且启动线程";
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
    threads_[i].reset(new qslary::Thread(std::bind(&Scheduler::run, this)));
    threads_[i]->start();
    threadsId_.push_back(threads_[i]->getId());
  }
}

void Scheduler::stop()
{
  autoStop_ = true;
  if (rootFiber_ && threadCount_ == 0 &&
      (rootFiber_->getState() == Fiber::State::INIT || rootFiber_->getState() == Fiber::TERM))
  {
    QSLARY_LOG_INFO(g_logger) << "stopped" << this;
    stopping_ = true;
    if (stopping())
    {
      return;
    }
  }

  if (rootThreadId != -1)
  {
    assert(GetThreadScheduler() == this);
  }
  else
  {
    assert(GetThreadScheduler() != this);
  }

  if (rootFiber_)
  {
    tickle();
  }
  if (rootFiber_)
  {
    if (!stopping())
    {
      rootFiber_->call();
    }
  }

  std::vector<qslary::Thread::ptr> temp;
  {
    qslary::MutexLockGuard lock(mutex_);
    temp.swap(threads_);
  }
  for (auto& i : temp)
  {
    i->join();
  }
}

void Scheduler::run()
{
  QSLARY_LOG_INFO(g_logger) << "entry Scheduler::run() ";
  setThreadScheduler();

  // 新线程 为其创建主线程
  if (qslary::CurrentThread::tid() != rootThreadId)
  {
    thread_scheduler_fiber = qslary::Fiber::GetThreadCurrentFiber().get();
  }
  
  QSLARY_LOG_DEBUG(g_logger)<<"创建idleFiber";
  Fiber::ptr idle_fiber(new Fiber(std::bind(&Scheduler::idle, this)));
  Fiber::ptr cb_fiber;
  FiberAndThread ft;
  while (true)
  {
    ft.reset();
    bool tickle_me = false;
    bool is_active = false;
    {
      qslary::MutexLockGuard lock(mutex_);
      auto it = fibers_.begin();
      while (it != fibers_.end())
      {
        if (it->threadID != -1 && it->threadID != qslary::CurrentThread::tid())
        {
          ++it;
          tickle_me = true;
          continue;
        }
        assert(it->fiber || it->callback);
        if (it->fiber && it->fiber->getState() == Fiber::State::EXEC)
        {
          ++it;
          continue;
        }
        ft = *it;
        fibers_.erase(it++);
        ++activeThreadCount_;
        is_active = true;
        break;
      }
      tickle_me |= it != fibers_.end();
    }

    if (tickle_me)
    {
      tickle();
    }
    
    // ft.fiber 为真
    if (ft.fiber && ft.fiber->getState() != Fiber::State::TERM && ft.fiber->getState() != Fiber::State::EXCEPTION)
    {

      // SwapIn 将当前协程与Scheduler::GetMainFiber切换
      ft.fiber->swapIn();
      --activeThreadCount_;
      if (ft.fiber->getState() == Fiber::State::READY)
      {
        scheduler(ft.fiber);
      }
      else if (ft.fiber->getState() != Fiber::State::TERM && ft.fiber->getState() != Fiber::State::EXCEPTION)
      {
        ft.fiber->state_ = Fiber::State::HOLD;
      }
      ft.reset();
    } // ft.fiber 为真

    else if (ft.callback) // ft.callback为真
    {
      if (cb_fiber)
      {
        cb_fiber->reset(ft.callback);
      }
      else if (ft.callback)
      {
        cb_fiber.reset(new Fiber(ft.callback));
      }

      ft.reset();
      cb_fiber->swapIn();
      --activeThreadCount_;
      if (cb_fiber->getState() == Fiber::State::READY)
      {
        scheduler(cb_fiber);
        cb_fiber.reset();
      }
      else if (cb_fiber->getState() == Fiber::State::EXCEPTION || cb_fiber->getState() == Fiber::State::TERM)
      {
        cb_fiber->reset(nullptr);
      }
      else
      {
        cb_fiber->state_ = Fiber::State::HOLD;
        cb_fiber.reset();
      }
    } // ft.callback为真

    else // 进入idle
    {
      if (is_active)
      {
        --activeThreadCount_;
        continue;
      }
      if (idle_fiber->getState() == Fiber::State::TERM)
      {
        QSLARY_LOG_INFO(g_logger) << " idle fiber 结束";
        break;
      }
      ++idleThreadCount_;
      idle_fiber->swapIn();
      --idleThreadCount_;
      QSLARY_LOG_DEBUG(g_logger)<<"从idle Fiber 返回 重新进入了Scheduler::run()";
      if (idle_fiber->getState() != Fiber::State::TERM && idle_fiber->getState() != Fiber::State::EXCEPTION)
      {
        idle_fiber->state_ = Fiber::State::HOLD;
      }
    }
  }
}

void Scheduler::tickle()
{
  QSLARY_LOG_INFO(g_logger) << "tickle";
}

bool Scheduler::stopping()
{
  qslary::MutexLockGuard lock(mutex_);
  return autoStop_ && stopping_ && fibers_.empty() && activeThreadCount_ == 0;
}

void Scheduler::idle()
{
  QSLARY_LOG_INFO(g_logger) << "idle";
  while (!stopping())
  {
    qslary::Fiber::YieldToHold();
  }
}

} // namespace qslary
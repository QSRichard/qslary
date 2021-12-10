#include "Timer.h"
#include "util.h"
#include "logger.h"

namespace qslary
{

static Logger::ptr g_logger=QSLARY_LOG_NAME("system");

bool Timer::Comparator::operator()(const Timer::ptr& lhs, const Timer::ptr& rhs) const
{

  if (lhs == nullptr && rhs == nullptr)
  {
    return false;
  }
  if (lhs == nullptr)
  {
    return true;
  }
  if (rhs == nullptr)
  {
    return false;
  }
  if (lhs->next_ < rhs->next_)
  {
    return true;
  }
  if (lhs->next_ > rhs->next_)
  {
    return false;
  }
  return lhs.get() < rhs.get();
}

Timer::Timer(std::uint64_t ms, std::function<void()> cb, bool recurring, TimerManger* timerManger)
    : ms_(ms), cb_(cb), recurring_(recurring), manager_(timerManger)
{
    next_=detail::GetCurrentMs()+ms_;
}

Timer::Timer(uint64_t next){
  next_=next;
}

bool Timer::cancel(){
  qslary::WriteLockGuard lock(manager_->mutex_);
  if(cb_){
    cb_=nullptr;
    auto it=manager_->timers_.find(shared_from_this());
    manager_->timers_.erase(it);
    return true;
  }
  return false;
} 
bool Timer::refersh(){
  qslary::WriteLockGuard lock(manager_->mutex_);
  if(!cb_){
    return false;
  }
  auto it=manager_->timers_.find(shared_from_this());
  if(it==manager_->timers_.end()){
    return false;
  }
  manager_->timers_.erase(it);
  next_=detail::GetCurrentMs()+ms_;
  manager_->timers_.insert(shared_from_this());
  return true;
}
bool Timer::reset(uint64_t ms, bool from_now){

  qslary::WriteLockGuard lock(manager_->mutex_);
  if(!cb_){
    return false;
  }
  auto it=manager_->timers_.find(shared_from_this());
  if(it==manager_->timers_.end()){
    return false;
  }
  manager_->timers_.erase(it);
  if(from_now){
    
  }
  uint64_t timebegin=0;
  if(from_now){
    timebegin=detail::GetCurrentMs();
  }else{
    timebegin=next_;
  }
  ms_=ms;
  next_=ms_+timebegin;
  manager_->timers_.insert(shared_from_this());
  return true;
}

TimerManger::TimerManger(){
  previousTime=detail::GetCurrentMs();
}

TimerManger::~TimerManger(){
  QSLARY_LOG_DEBUG(g_logger)<<"TimerManager 析构";

}

Timer::ptr TimerManger::addTimer(std::uint64_t ms, std::function<void ()> cb,bool recurring){

  QSLARY_LOG_DEBUG(g_logger)<<"进入了addTimer";
    Timer::ptr timer(new Timer(ms,cb,recurring,this));
    qslary::WriteLockGuard lock(mutex_);
    auto it=timers_.insert(timer).first;
    bool at_front=(it==timers_.begin());
    if(at_front){
        onTimerInsertAtFront();
    }
    return timer;
}

static void OnTimer(std::weak_ptr<void> weak,std::function<void()> cb){
    auto tmp=weak.lock();
    if(tmp){
        cb();
    }
}

Timer::ptr TimerManger::addConditionTimer(std::uint64_t ms, std::function<void ()> cb, std::weak_ptr<void> weak_cond,bool recurring){

    return addTimer(ms, std::bind(&OnTimer,weak_cond, cb),recurring);
}

uint64_t TimerManger::getNextTimer(){
  qslary::ReadLockGuard lock(mutex_);
  if(timers_.empty()){
    return ~0ull;
  }
  const Timer::ptr& next=*timers_.begin();
  uint64_t now_ms=detail::GetCurrentMs();
  if(now_ms>=next->next_){
    return 0;
  }
  return next->next_-now_ms;
}

void TimerManger::listExpiredCallerBack(std::vector<std::function<void()>>& callbacks){
  
  uint64_t now_ms=detail::GetCurrentMs();
  std::vector<Timer::ptr> expired;;
  {
    qslary::ReadLockGuard lock(mutex_);
    if(timers_.empty()){
      return;
    }
  }

  qslary::WriteLockGuard lock(mutex_);
  bool rollover=detectClockRollover(now_ms);
  Timer::ptr now_timer(new Timer(now_ms));

  // 如果rollover 为真 设置it为timers_.end()
  // TODO 
  auto it=rollover?timers_.end():timers_.lower_bound(now_timer);
  while(it != timers_.end() && (*it)->next_==now_ms){
    ++it;
  }
  expired.insert(expired.begin(),timers_.begin(),it);
  timers_.erase(timers_.begin(),it);
  callbacks.reserve(expired.size());
  for(auto& timer : expired){
    callbacks.push_back(timer->cb_);
    if(timer->recurring_){
      timer->next_=now_ms+timer->ms_;
      timers_.insert(timer);
    }else{
      // 防止回调函数中可能使用了智能指针
      timer->cb_=nullptr;
    }
  }
}

bool TimerManger::detectClockRollover(uint64_t now_ms){
  bool rollover=false;
  // 当now_ms小于上次时间 并且比上次时间小一个小时 认为修改过服务器时间
  if(now_ms < previousTime && now_ms < (previousTime-60*60*1000)){
    rollover=true;
  }
  previousTime=now_ms;
  return rollover;
}

} // namespace qslary
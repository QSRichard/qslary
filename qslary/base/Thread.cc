#include "qslary/base/CurrentThread.h"
#include "qslary/base/Exception.h"
#include "qslary/base/Logger.h"
#include "qslary/base/Thread.h"

#include <ctime>
#include <errno.h>
#include <linux/unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <type_traits>
#include <unistd.h>
namespace qslary {

namespace detail {

pid_t gettid() { return static_cast<pid_t>(::syscall(SYS_gettid)); }

void afterFork() {
  qslary::CurrentThread::t_cachedTid = 0;
  qslary::CurrentThread::t_threadName = "main";
  CurrentThread::tid();
}

class ThreadNameInitializer {
  public:
  ThreadNameInitializer() {
    qslary::CurrentThread::t_threadName = "main";
    qslary::CurrentThread::tid();
    pthread_atfork(NULL, NULL, &afterFork);
  }
};
ThreadNameInitializer init;

struct ThreadData {

  typedef qslary::Thread::ThreadFunc ThreadFunc;

  ThreadFunc func_;
  string name_;
  pid_t *tid_;
  CountDownLatch *latch_;

  ThreadData(ThreadFunc func, const string &name, pid_t *tid,
             CountDownLatch *latch)
      : func_(func), name_(name), tid_(tid), latch_(latch) {}

  void runInThread() {

    *tid_ = qslary::CurrentThread::tid();
    tid_ = NULL;
    latch_->countDown();
    latch_ = NULL;

    qslary::CurrentThread::t_threadName =
        name_.empty() ? "qslary thread" : name_.c_str();
    ::prctl(PR_SET_NAME, qslary::CurrentThread::t_threadName);
    try {
      func_();
      qslary::CurrentThread::t_threadName = "finished";
    } catch (const Exception &ex) {
      qslary::CurrentThread::t_threadName = "crashed";
      QSLARY_LOG_ERROR(QSLARY_LOG_ROOT())
          << "exception caught in Thread " << name_.c_str() << "reason "
          << ex.what() << "stack trace" << ex.stackTrace();
      abort();
    } catch (const std::exception &ex) {
      qslary::CurrentThread::t_threadName = "crashed";
      QSLARY_LOG_ERROR(QSLARY_LOG_ROOT())
          << "exception caught in Thread " << name_.c_str() << "reason "
          << ex.what();
      abort();
    } catch (...) {
      qslary::CurrentThread::t_threadName = "crashed";
      QSLARY_LOG_ERROR(QSLARY_LOG_ROOT())
          << "unknown exception caught in thread " << name_.c_str();
      throw;
    }
  }
};

void *startThread(void *obj) {
  ThreadData *data = static_cast<ThreadData *>(obj);
  data->runInThread();
  delete data;
  return NULL;
}
} // namespace detail

void CurrentThread::cacheTid() {

  if (t_cachedTid == 0) {
    t_cachedTid = detail::gettid();
    t_tidStringLength =
        snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
  }
}

bool CurrentThread::isMainThread() { return tid() == ::getpid(); }

AtomicInt64 Thread::threadNum;

Thread::Thread(ThreadFunc func, const string &n)
    : pthreadId_(0), tid_(0), func_(func), latch_(1), name_(n), started_(false),
      joined_(false) {
  setDefaultName();
}

Thread::~Thread() {
  if (started_ && !joined_) {
    pthread_detach(pthreadId_);
  }
}

void Thread::setDefaultName() {
  int64_t num = threadNum.incrementAndGet();
  if (name_.empty()) {
    char buf[32];
    snprintf(buf, sizeof buf, "Thread%ld", num);
    name_ = buf;
  }
}

void Thread::start() {
  assert(!started_);
  started_ = true;
  detail::ThreadData *data =
      new detail::ThreadData(func_, name_, &tid_, &latch_);
  if (pthread_create(&pthreadId_, NULL, &detail::startThread, data)) {
    started_ = false;
    delete data;
    QSLARY_LOG_ERROR(QSLARY_LOG_ROOT()) << " Failed in pthread_create";
  } else {
    latch_.wait();
    assert(tid_ > 0);
    // QSLARY_LOG_ERROR(QSLARY_LOG_ROOT()) <<
    // "staered..."<<qslary::CurrentThread::tid();
  }
}

int Thread::join() {
  assert(started_);
  assert(!joined_);
  joined_ = true;
  // TAG
  // std::cout<<"tid ======== "<<tid_<<std::endl;
  return pthread_join(pthreadId_, NULL);
  // return pthread_detach(pthreadId_);
}

// const std::string &Thread::getThreadName()
// {
//   return name_;
// }

} // namespace qslary
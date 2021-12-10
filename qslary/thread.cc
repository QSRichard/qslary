#include "thread.h"
#include "currenthread.h"
#include "logger.h"
#include "Exception.h"

#include <type_traits>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <sys/prctl.h>
namespace qslary
{

  namespace detail
  {

    pid_t gettid()
    {
      return static_cast<pid_t>(::syscall(SYS_gettid));
    }

    void afterFork()
    {
      qslary::CurrentThread::t_cachedTid = 0;
      qslary::CurrentThread::t_threadName = "main";
      CurrentThread::tid();
    }

    class ThreadNameInitializer
    {
    public:
      ThreadNameInitializer()
      {
        qslary::CurrentThread::t_threadName = "main";
        qslary::CurrentThread::tid();
        pthread_atfork(NULL, NULL, &afterFork);
      }
    };
    ThreadNameInitializer init;

    struct ThreadData
    {

      typedef qslary::Thread::ThreadFunc ThreadFunc;

      ThreadFunc m_func;
      string m_name;
      pid_t *m_tid;
      CountDownLatch *m_latch;

      ThreadData(ThreadFunc func,
                 const string &name,
                 pid_t *tid,
                 CountDownLatch *latch) : m_func(func),
                                          m_name(name),
                                          m_tid(tid),
                                          m_latch(latch) {}

      void runInThread()
      {

        *m_tid = qslary::CurrentThread::tid();
        m_tid = NULL;
        m_latch->countDown();
        m_latch = NULL;

        qslary::CurrentThread::t_threadName = m_name.empty() ? "qslary thread" : m_name.c_str();
        ::prctl(PR_SET_NAME, qslary::CurrentThread::t_threadName);
        try
        {
          m_func();
          qslary::CurrentThread::t_threadName = "finished";
        }
        catch (const Exception &ex)
        {
          qslary::CurrentThread::t_threadName = "crashed";
          QSLARY_LOG_ERROR(QSLARY_LOG_ROOT()) << "exception caught in Thread "
                                              << m_name.c_str()
                                              << "reason " << ex.what()
                                              << "stack trace" << ex.stackTrace();
          abort();
        }
        catch (const std::exception &ex)
        {
          qslary::CurrentThread::t_threadName = "crashed";
          QSLARY_LOG_ERROR(QSLARY_LOG_ROOT()) << "exception caught in Thread "
                                              << m_name.c_str()
                                              << "reason " << ex.what();
          abort();
        }
        catch (...)
        {
          qslary::CurrentThread::t_threadName = "crashed";
          QSLARY_LOG_ERROR(QSLARY_LOG_ROOT()) << "unknown exception caught in thread "
                                              << m_name.c_str();
          throw;
        }
      }
    };

    void *startThread(void *obj)
    {
      ThreadData *data = static_cast<ThreadData *>(obj);
      data->runInThread();
      delete data;
      return NULL;
    }
  } // namespace detail end

  void CurrentThread::cacheTid()
  {

    if (t_cachedTid == 0)
    {
      t_cachedTid = detail::gettid();
      t_tidStringLength = snprintf(t_tidString, sizeof t_tidString, "%5d ", t_cachedTid);
    }
  }

  bool CurrentThread::isMainThread()
  {
    return tid() == ::getpid();
  }

  Semaphore::Semaphore(uint32_t count)
  {
    if (sem_init(&semaphore_, 0, count))
    {
      throw std::logic_error("sem_init error");
    }
  }

  Semaphore::~Semaphore()
  {
    if (sem_destroy(&semaphore_))
      ;
  }

  void Semaphore::wait()
  {
    if (sem_wait(&semaphore_))
    {
      throw std::logic_error("sem_wait error");
    }
  }

  void Semaphore::notify()
  {
    if (sem_post(&semaphore_))
    {
      throw std::logic_error("sem_post error");
    }
  }

  AtomicInt64 Thread::threadNum;

  Thread::Thread(ThreadFunc func, const string &n)
      : m_pthreadId(0),
        m_tid(0),
        m_func(func),
        m_latch(1),
        m_name(n),
        m_started(false),
        m_joined(false)
  {
    setDefaultName();
  }

  Thread::~Thread()
  {
    if (m_started && !m_joined)
    {
      pthread_detach(m_pthreadId);
    }
  }

  void Thread::setDefaultName()
  {
    int64_t num = threadNum.incrementAndGet();
    if (m_name.empty())
    {
      char buf[32];
      snprintf(buf, sizeof buf, "Thread%ld", num);
      m_name = buf;
    }
  }

  void Thread::start()
  {
    assert(!m_started);
    m_started = true;
    detail::ThreadData *data = new detail::ThreadData(m_func, m_name, &m_tid, &m_latch);
    if (pthread_create(&m_pthreadId, NULL, &detail::startThread, data))
    {

      m_started = false;
      delete data;
      QSLARY_LOG_ERROR(QSLARY_LOG_ROOT()) << " Failed in pthread_create";
    }
    else
    {
      m_latch.wait();
      assert(m_tid > 0);
      // QSLARY_LOG_ERROR(QSLARY_LOG_ROOT()) << "staered..."<<qslary::CurrentThread::tid();
    }
  }

  int Thread::join()
  {
    assert(m_started);
    assert(!m_joined);
    m_joined = true;
    return pthread_join(m_pthreadId, NULL);
  }

  // const std::string &Thread::getThreadName()
  // {
  //   return m_name;
  // }

} // namespace qslary
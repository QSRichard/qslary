#ifndef __QSLARY_MUTEX_H_
#define __QSLARY_MUTEX_H_

#include "noncopyable.h"
#include "currenthread.h"

#include <pthread.h>
#include <assert.h>

#ifdef CHECK_PTHREAD_RETURN_VALUE

#ifdef NDEBUG
__BEGIN_DECLS
extern void __assert_perror_fail(int errnum,
                                 const char *file,
                                 unsigned int line,
                                 const char *function) noexcept __attribute__((__noreturn__));

__END_DECLS
#endif

#define MCHECK(ret) ({ __typeof__ (ret) errnum=(ret); \
                        if(__builtin_expect(errnum!=0,0)) \
                          __assert_perror_fail(errnum,__FILE__,__LINE__,__func__); })

#else // CHECK_PTHREAD_RETURN_VALUE

#define MCHECK(ret) ({ __typeof__ (ret) errnum=(ret); \
                       assert(errnum==0); (void)errnum; })

#endif // CHECK_PTHREAD_RETURN_VALUE

namespace qslary
{

  class MutexLock : noncopyable
  {
  public:
    MutexLock() : m_holder(0)
    {
      MCHECK(pthread_mutex_init(&m_mutex, NULL));
    }

    ~MutexLock()
    {
      assert(m_holder == 0);
      MCHECK(pthread_mutex_destroy(&m_mutex));
    }

    bool isLockedByThisThread() const
    {
      return m_holder == CurrentThread::tid();
    }

    void assertLocked() const
    {
      assert(isLockedByThisThread());
    }

    void assignHolder()
    {
      m_holder = CurrentThread::tid();
    }

    void unassignHolder()
    {
      m_holder = 0;
    }

    void lock()
    {
      MCHECK(pthread_mutex_lock(&m_mutex));
      assignHolder();
    }

    void unlock()
    {
      unassignHolder();
      MCHECK(pthread_mutex_unlock(&m_mutex));
    }

    pthread_mutex_t *getPthreadMutex()
    {
      return &m_mutex;
    }

  private:
    friend class Condition;

    class UnassignGuard
    {

    public:
      explicit UnassignGuard(MutexLock &owner) : m_owner(owner)
      {
        m_owner.unassignHolder();
      }

      ~UnassignGuard()
      {
        m_owner.assignHolder();
      }

    private:
      MutexLock &m_owner;
    };

    pthread_mutex_t m_mutex;
    pid_t m_holder;
  };

  class SpinLock
  {
  public:
    SpinLock()
    {
      pthread_spin_init(&spinLock_, 0);
    }
    ~SpinLock()
    {
      pthread_spin_destroy(&spinLock_);
    }
    void lock()
    {
      pthread_spin_lock(&spinLock_);
    }
    void unlock()
    {
      pthread_spin_unlock(&spinLock_);
    }

  private:
    pthread_spinlock_t spinLock_;
  };

  class NullMutex
  { // for testing thread safety
  public:
    NullMutex() { ; };
    ~NullMutex() { ; };
    void lock() { ; };
    void unlock() { ; };

  private:
    int val_;
  };

  class RWMutex : noncopyable
  {

  public:
    RWMutex()
    {
      pthread_rwlock_init(&rwlock_, nullptr);
    }

    ~RWMutex()
    {
      pthread_rwlock_destroy(&rwlock_);
    }

    void rlock()
    {
      pthread_rwlock_rdlock(&rwlock_);
    }

    void wlock()
    {
      pthread_rwlock_wrlock(&rwlock_);
    }
    void unlock()
    {
      pthread_rwlock_unlock(&rwlock_);
    }

  private:
    pthread_rwlock_t rwlock_;
  };

  class NullRWMutex
  { // for testing thread safety
  public:
    NullRWMutex() { ; };
    ~NullRWMutex(){};
    ;
    void rlock() { ; };
    void wlock() { ; };
    void unlock() { ; };

  private:
    int val_;
  };

  template <typename T>
  struct ScopedLockImpl
  {
  public:
    ScopedLockImpl(T &mutex) : mutex_(mutex)
    {
      mutex_.lock();
      locked_ = true;
    }
    ~ScopedLockImpl()
    {
      unlock();
    }

    void lock()
    {
      if (!locked_)
      {
        mutex_.lock();
        locked_ = true;
      }
    }

    void unlock()
    {
      if (locked_)
      {
        mutex_.unlock();
        locked_ = false;
      }
    }

  private:
    T &mutex_;
    bool locked_;
  };

  template <typename T>
  struct ReadScopedLockImpl
  {
  public:
    ReadScopedLockImpl(T &mutex) : mutex_(mutex)
    {
      mutex_.rlock();
      locked_ = true;
    }
    ~ReadScopedLockImpl()
    {
      unlock();
    }

    void lock()
    {
      if (!locked_)
      {
        mutex_.rlock();
        locked_ = true;
      }
    }

    void unlock()
    {
      if (locked_)
      {
        mutex_.unlock();
        locked_ = false;
      }
    }

  private:
    T &mutex_;
    bool locked_;
  };

  template <typename T>
  struct WriteScopedLockImpl
  {
  public:
    WriteScopedLockImpl(T &mutex) : mutex_(mutex)
    {
      mutex_.wlock();
      locked_ = true;
    }
    ~WriteScopedLockImpl()
    {
      unlock();
    }

    void lock()
    {
      if (!locked_)
      {
        mutex_.wlock();
        locked_ = true;
      }
    }

    void unlock()
    {
      if (locked_)
      {
        mutex_.unlock();
        locked_ = false;
      }
    }

  private:
    T &mutex_;
    bool locked_;
  };

  class MutexLockGuard : noncopyable
  {

  public:
    MutexLockGuard(MutexLock &lock) : m_mutex(lock)
    {
      m_mutex.lock();
    }

    ~MutexLockGuard()
    {
      m_mutex.unlock();
    }

  private:
    MutexLock &m_mutex;
  };

  typedef ReadScopedLockImpl<RWMutex> ReadLockGuard;
  typedef WriteScopedLockImpl<RWMutex> WriteLockGuard;

  typedef ReadScopedLockImpl<NullRWMutex> NullReadLockGuard;   // for testing thread safety
  typedef WriteScopedLockImpl<NullRWMutex> NullWriteLockGuard; // for testing thread safety

  typedef ScopedLockImpl<NullMutex> NullMutexGuard;

} // namespace qslary

#define MutexLockGuard(x) error "Missing guard object name"
#define ReadLockGuard(x) error "Missing guard object name"
#define WriteLockGuard(x) error "Missing guard object name"

#endif // __QSLARY_MUTEX_H_
#ifndef __QSLARY_CURRENT_THREAD_H_
#define __QSLARY_CURRENT_THREAD_H_

#include "types.h"

namespace qslary
{
  namespace CurrentThread
  {

    extern __thread int t_cachedTid;
    extern __thread char t_tidString[32];
    extern __thread int t_tidStringLength;
    extern __thread const char *t_threadName;

    void cacheTid();

    bool isMainThread();

    inline int tid()
    {
      // 当t_cacheTid等于0时执行if里面的指令，但是大概率不需要执行if里面的指令
      if (__builtin_expect(t_cachedTid == 0, 0))
      {
        cacheTid();
      }
      return t_cachedTid;
    }

    inline const char *tidString()
    {
      return t_tidString;
    }

    inline int tidStringLength()
    {
      return t_tidStringLength;
    }

    inline const char *name()
    {
      return t_threadName;
    }

    std::string stackTrace(bool demangle);
  }
}

#endif
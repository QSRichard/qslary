#ifndef __QSLARY__UTIL_H 
#define __QSLARY__UTIL_H
#include <sys/syscall.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

namespace qslary{
    namespace detail{
        static pid_t getThreadId(){
            return syscall(SYS_gettid);
        }
        static uint32_t getFiberId(){
            return 0;
        }
    }

}

#endif
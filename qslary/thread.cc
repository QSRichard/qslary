#include "thread.h"
#include "logger.h"
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <linux/unistd.h>
#include <sys/prctl.h>
namespace qslary{

    pid_t gettid(){
        return static_cast<pid_t>(::syscall(SYS_gettid));
    }


    struct ThreadData{

        Thread::ThreadFunc m_func;
        std::string m_name;
        pid_t* m_tid;
        

    };
}
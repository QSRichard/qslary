#ifndef __QSLARY_HOOK_H_
#define __QSLARY_HOOK_H_

#include <unistd.h>

namespace qslary{

    bool is_hook_enable();
    void set_hook_enable(bool flag);
}

extern "C"{
    typedef unsigned int (*sleep_fun)(unsigned int seconds);
    extern sleep_fun sleep_f;

    typedef int (*usleep_fun) (useconds_t usec);
    extern usleep_fun unsleep_f;

}

#endif
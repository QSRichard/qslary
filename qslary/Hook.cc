
#include "Hook.h"
#include "Fiber.h"
#include "IOManager.h"


namespace qslary{

    static thread_local bool thread_hook_enable=false;

    #define HOOK_FUN(xx) \
        XX(sleep) \
        XX(unsleep)

    void hook_init(){
        static bool is_inited=false;
        if(is_inited){
            return;
        }
        #define XX(name) name ## _f = (name ## _fun)dlsym(RTLD_NEXT, #name);
            HOOK_FUN(xx);
        #undef XX;

    }

    struct _HookIniter{
        _HookIniter(){
            hook_init();
        }
    };

    static _HookIniter static_hook_initer;

    bool is_hook_enable(){
        return thread_hook_enable;
    }

    void set_hook_enable(bool flag){
        thread_hook_enable=flag;
    }
}

extern "C"{
    #define XX(name) name ## _fun name ## _f =nullptr;
        HOOK_FUN(xx)
    #undef XX

    unsigned int sleep(unsigned int seconds){
        if(!qslary::thread_hook_enable){
            return sleep_f(seconds);
        }

        qslary::Fiber::ptr fiber=qslary::Fiber::GetThreadCurrentFiber();
        qslary::IOManager* iom=qslary::IOManager::Get

    }
}

typedef unsigned int (*sleep_fun)(unsigned int seconds);
extern sleep_fun sleep_f;

typedef int (*usleep_fun)(useconds_t usec);
extern usleep_fun unsleep_f;

#ifndef __QSLARY_THREAD_h
#define __QSLARY_THREAD_h

#include <functional>
#include <memory>
#include <thread>
#include <string>
#include <pthread.h>
namespace qslary{

    class Thread{
        public:
            typedef std::shared_ptr<Thread> ptr;
            typedef std::function<void()> ThreadFunc;
            explicit Thread(const ThreadFunc& func,const std::string& name=std::string());
            Thread(const Thread& rhs)=delete;
            Thread&operator=(const Thread& rhs)=delete;
            ~Thread();
            void start();
            int join();
            bool started() const {return m_started;};

        private:
            bool m_started;
            bool m_joined;
            pthread_t m_pthreadId;
            pid_t m_tid;

            ThreadFunc m_func;
            std::string m_name;
            
    };
}

#endif
#ifndef __QSLARY__UTIL_H
#define __QSLARY__UTIL_H
#include <sys/syscall.h>
#include <pthread.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <cxxabi.h>
#include <memory>
#include <stdlib.h>
namespace qslary{
    namespace detail{

        pid_t getThreadId();
        uint32_t getFiberId();

        std::shared_ptr<char> cppDemangled(const char *abiName);

        #define DEMANGLED_CLASS_NAME(somePointer) ((const char *)qslary::detail::cppDemangled(typeid(*somePointer).name()).get())
    }
}

#endif
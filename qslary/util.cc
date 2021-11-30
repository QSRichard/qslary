#include "util.h"

namespace qslary{
    namespace detail{

        pid_t getThreadId(){
            return syscall(SYS_gettid);
        }

        uint32_t getFiberId(){
            return 0;
        }

        std::shared_ptr<char> cppDemangled(const char *abiName)
        {
            int status;
            char *ret = abi::__cxa_demangle(abiName, 0, 0, &status);
            std::shared_ptr<char> retval;
            retval.reset((char *)ret, [](char *mem)
                         {
                             if (mem)
                                 free((void *)mem);
                         });
            return retval;
        }
    }
}

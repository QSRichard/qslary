#include "qslary/base/util.h"
#include "qslary/base/Fiber.h"

namespace qslary
{
  namespace detail
  {

    pid_t getThreadId()
    {
      return syscall(SYS_gettid);
    }

    uint64_t getFiberId()
    {
      return qslary::Fiber::getFiberId();
    }

    uint64_t GetCurrentMs(){

      struct timeval tv;
      gettimeofday(&tv, NULL);
      return tv.tv_sec*1000+tv.tv_usec;
    }


  uint64_t GetCurrentUs(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_usec*1000*1000+tv.tv_usec*1000;
  }
    std::shared_ptr<char> cppDemangled(const char *abiName)
    {
      int status;
      char *ret = abi::__cxa_demangle(abiName, 0, 0, &status);
      std::shared_ptr<char> retval;
      retval.reset((char *)ret, [](char *mem)
                   {
                             if (mem)
                                 free((void *)mem); });
      return retval;
    }
  }
}

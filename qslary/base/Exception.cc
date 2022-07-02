#include "qslary/base/Exception.h"
#include "qslary/base/CurrentThread.h"

namespace qslary
{

  Exception::Exception(string msg) : m_message(std::move(msg)),
                                     m_stack(CurrentThread::stackTrace(/*demangled=*/true))
  {
  }

} // namespace qslary end
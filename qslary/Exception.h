#ifndef __QSALRY_EXCEPTION_H_
#define __QSLARY_EXCEPTION_H_

#include "types.h"
#include <exception>

namespace qslary
{

  class Exception : std::exception
  {
  public:
    Exception(string what);
    ~Exception() noexcept override = default;

    const char *what() const noexcept override
    {
      return m_message.c_str();
    }

    const char *stackTrace() const noexcept
    {
      return m_stack.c_str();
    }

  private:
    string m_message;
    string m_stack;
  };
} // namespace qslary

#endif // __QSLARY_EXCEPTION_H_
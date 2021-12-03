
#ifndef __QSLARY_NONCOPYABLE_H
#define __QSLARY_NONCOPYABLE_H

namespace qslary
{

  class noncopyable
  {
  public:
    noncopyable(const noncopyable &) = delete;
    void operator=(const noncopyable &) = delete;
    noncopyable(const noncopyable &&) = delete;

  protected:
    noncopyable() = default;
    ~noncopyable() = default;
  };

}

#endif
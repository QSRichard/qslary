#ifndef __QSLARY_SINGLETON_H
#define __QSLARY_SINGLETON_H
#include <memory>
namespace qslary
{
  template <class T, class X = void, int N = 0>
  class Singleton
  {
  public:
    static T *getInstance()
    {
      static T v;
      return &v;
    }
  };
  template <class T, class X = void, int N = 0>
  class SingletonPtr
  {

  public:
    static std::shared_ptr<T> getInstance()
    {
      static std::shared_ptr<T> v(new T);
      return v;
    }
  };
}

#endif
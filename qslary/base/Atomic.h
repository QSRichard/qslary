/**
 * @file Atomic.h
 * @author qiao (1325726511@qq.com)
 * @brief
 * @version 0.1
 * @date 2021-12-04
 *
 * @copyright Copyright (c) 2021
 *
 */

#ifndef __QSLARY_ATOMIC_H_
#define __QSLARY_ATOMIC_H_

#include "qslary/base/noncopyable.h"

#include <stdint.h>

namespace qslary
{

namespace detail
{

template <typename T> class AtomicIntegerT : noncopyable
{
public:
    AtomicIntegerT() : value_(0) {}

    T get()
    {
        // __sync_val_compare_and_swap 是gcc定义的函数 在clang中也得到了支持
        return __sync_val_compare_and_swap(&value_, 0, 0);
    }

    T getAndAdd(T x) { return __sync_fetch_and_add(&value_, x); }

    T addAndGet(T x) { return getAndAdd(x) + x; }

    T incrementAndGet() { return addAndGet(1); }

    T decrementAndGet() { return addAndGet(-1); }

    void add() { getAndAdd(1); }

    void increment() { incrementAndGet(); }

    void decrement() { decrementAndGet(); }

    T getAndSet(T newValue)
    {
        return __sync_lock_test_and_set(&value_, newValue);
    }

private:
    // TODO volatile 的用法和作用
    volatile T value_;
};

} // namespace detail

typedef detail::AtomicIntegerT<int32_t> AtomicInt32;
typedef detail::AtomicIntegerT<int64_t> AtomicInt64;

} // namespace qslary

#endif // __QSLARY_ATOMIC_H_
#ifndef __QSLARY_MACRO_H_
#define __QSLARY_MACRO_H_


#if defined __GNUC__ || defined __llvm__

#define QSLARY_LICKLY(x) __builtin_expect(!!(x), 1)
#define QSLARY_UNLICKLY(x) __builtin_expect(!!(x), 0)

#else

#define QSLARY_LICKLY(x) (x)
#define QSLARY_UNLICKLY(x) (x)

#endif


#endif // __QSLARY_MACRO_H_
#include <functional>
#ifndef __QSLARY_ALISE_CONFIG_H__
typedef std::function<void()> WriteCallback;
typedef std::function<void()> ReadCallback;
static const int DefaultEpollTime = 1000;

#endif // __QSLARY_ALISE_CONFIG_H__
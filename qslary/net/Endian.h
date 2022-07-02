/**
 * @file Endian.h
 * @author qiao (1325726511@qq.com)
 * @brief  转换字节序
 * @version 0.1
 * @date 2021-12-11
 *
 * @copyright Copyright (c) 2021
 *
 */
#ifndef __QSLARY_ENDIAN_H_
#define __QSLARY_ENDIAN_H_

#include <byteswap.h>
#include <cstdint>
#include <stdint.h>
#include <type_traits>

#define QSLARY_LITTLE_ENDIAN 1
#define QSLARY_BIG_ENDIAN 2

namespace qslary
{
template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type byteswap(T value)
{
  return (T)bswap_64((uint64_t)value);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type byteswap(T value)
{
  return (T)bswap_32((uint32_t)value);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type byteswap(T value)
{
  return (T)bswap_16((uint16_t)value);
}

template <class T>
typename std::enable_if<sizeof(T) == sizeof(uint8_t), T>::type byteswap(T value)
{
  return (T)(value);
}

#if BYTE_ORDER == BIG_ENDIAN
#define QSLARY_BYTE_ORDER QSLARY_BIG_ENDIAN
#else
#define QSLARY_BYTE_ORDER QSLARY_LITTLE_ENDIAN
#endif

// 网络字节序为大端 因此在主机上需要调用byteOnLittleEndian()
// 当机器是大端时 此函数什么都不做 当机器是小端时 此函数进行byte swap

#if QSLARY_BYTE_ORDER == QSLARY_BIG_ENDIAN


/**
 * @brief 当发送端是大端时 调用此方法
 * 
 */
template <class T>
T byteswapOnLittleEndian(T val)
{
  return val;
}

/**
 * @brief 当发送端时小端时 调用此方法
 * 
 */
template <class T>
T byteswapOnBigEndian(T val)
{
  return byteswap(val);
}

#else

/**
 * @brief 当发送端是大端时 调用此方法
 *
 */
template <class T>
T byteswapOnLittleEndian(T val)
{
  return byteswap(val);
}

/**
 * @brief 当发送端时小端时 调用此方法
 *
 */
template <class T>
T byteswapOnBigEndian(T val)
{
  return val;
}
#endif

} // namespace qslary

#endif
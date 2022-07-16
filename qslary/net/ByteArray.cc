#include "qslary/net/ByteArray.h"
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <math.h>
#include <sstream>
#include <string.h>
#include <iostream> 
#include "endian.h"
#include <assert.h>
#include "qslary/base/copyable.h"
#include "qslary/net/Endian.h"

namespace qslary
{

ByteArray::ByteArray(size_t base_size)
{
  mData.resize(base_size);
}

ByteArray::~ByteArray()
{
}

static uint32_t EncodeZigzag32(const int32_t& v)
{
  if (v < 0)
  {
    return ((uint32_t)(-v)) * 2 - 1;
  }
  else
  {
    return v * 2;
  }
}

static uint64_t EncodeZigzag64(const int64_t& v)
{
  if (v < 0)
  {
    return ((uint64_t)(-v)) * 2 - 1;
  }
  else
  {
    return v * 2;
  }
}

static int32_t DecodeZigzag32(const uint32_t& v) { return (v >> 1) ^ -(v & 1); }

static int64_t DecodeZigzag64(const uint64_t& v) { return (v >> 1) ^ -(v & 1); }

void ByteArray::clear()
{
  mReadIndex = 0;
  mWriteIndex = 0;
  mData.clear();
}

size_t ByteArray::GetReadBuffer(iovec& iov, size_t len)
{
  size_t realSize = len > ReadableSize()?ReadableSize():len;
  iov.iov_base = const_cast<char*>(Peek());
  iov.iov_len = realSize;
  mReadIndex += realSize;
  return realSize;
}

size_t ByteArray::Read(void* buf, size_t size)
{
  size_t realRead = size > ReadableSize()?ReadableSize():size;
  if (realRead == 0)
  {
    return realRead;
  }
  memcpy(buf, mData.data() + mReadIndex, realRead);
  mReadIndex += realRead;
  return realRead;
}

size_t ByteArray::Write(const void* buf, size_t size)
{
  if (WriteableSize() < size)
  {
    MakeSpace(size);
  }
  const char* tmp = static_cast<const char*> (buf);
  std::copy(tmp, tmp + size, mData.data() + mWriteIndex);
  mWriteIndex += size;
  return size;
}

void ByteArray::MakeSpace(size_t size)
{
  if (PrependableSize() + WriteableSize() < size + kPreHeadSize)
  {
    mData.resize(mWriteIndex + size);
    return;
  }
  size_t readable = ReadableSize();
  std::copy(mData.data() + mReadIndex, mData.data()+mWriteIndex, mData.data() + kPreHeadSize);
  mReadIndex = kPreHeadSize;
  mWriteIndex = mReadIndex + readable;
}

std::string ByteArray::toString()
{
  std::string str;
  str.resize(ReadableSize());
  if (str.empty())
  {
    return str;
  }
  Read(&str[0], str.size());
  return str;
}

std::string ByteArray::toHexString()
{
  std::string str = toString();
  std::stringstream ss;

  for (size_t i = 0; i < str.size(); ++i)
  {
    if (i > 0 && i % 32 == 0)
    {
      ss << std::endl;
    }
    ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)str[i] << " ";
  }

  return ss.str();
}

} // namespace sylar

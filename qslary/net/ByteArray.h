
#ifndef __QSYLARY_BYTEARRAY_H__
#define __QSYLARY_BYTEARRAY_H__

#include <bits/types/struct_iovec.h>
#include <cstddef>
#include <memory>
#include <stdint.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

namespace qslary
{
static const int kInitSize = 8192;
static const int kPreHeadSize = 16;
class ByteArray
{
public:
  typedef std::shared_ptr<ByteArray> ptr;


  ByteArray(size_t base_size = 4096);
  ~ByteArray();

  template<class T>
  void WriteInt(T val)
  {
    Write(&val, sizeof(val));
  }

  // template<class T>
  // void WriteCompressInt(T val);

  template<class T>
  T ReadInt()
  {
    T tmp;
    Read(&tmp, sizeof(T));
    return tmp;
  }

  void writeStringF64(const std::string& value);
  void writeStringVint(const std::string& value);

  std::string readStringF64();
  std::string readStringVint();

  void clear();

  size_t GetReadBuffer(iovec& iov, size_t len);
  size_t Read(void* buf, size_t size);
  size_t Write(const void* buf, size_t size);
  void MakeSpace(size_t size);

  bool isLittleEndian() const;
  void SetIsLittleEndian(bool val);

  std::string toString();
  std::string toHexString();

  size_t ReadableSize() const {return mWriteIndex - mReadIndex; }
  size_t WriteableSize() const {return mData.size() - mWriteIndex; }
  size_t PrependableSize() const {return mReadIndex;}
  const char* Peek() const {return mData.data() + mReadIndex; }


private:
  std::vector<char> mData;
  size_t mReadIndex;
  size_t mWriteIndex;
};

} // namespace sylar

#endif

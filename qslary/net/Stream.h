#ifndef __QSLARY_STREAM_H_
#define __QSLARY_STREAM_H_

#include "qslary/net/ByteArray.h"
#include "qslary/net/Endian.h"
#include <cstddef>
#include <memory>
namespace qslary
{

class Stream
{
public:
    typedef std::shared_ptr<Stream> ptr;

    virtual ~Stream(){};

    //  ***********************     READ     ***********************  //

    virtual int read(void *buffer, size_t length) = 0;

    virtual int read(ByteArray::ptr bytearray, size_t length) = 0;

    // 以下两个虚函数使用以上两个纯虚函数辅助实现
    virtual int readFixSize(void *buffer, size_t length);
    virtual int readFixSize(ByteArray::ptr bytearray, size_t length);

    //  ***********************     WRITE     ***********************  //

    virtual int write(const void *buffer, size_t length) = 0;
    virtual int write(ByteArray::ptr bytearray, size_t length) = 0;

    // 以下两个虚函数使用以上两个纯虚函数辅助实现
    virtual int writeFixSize(const void *buffer, size_t length);
    virtual int writeFixSize(ByteArray::ptr bytearray, size_t length);

    //  ***********************     CLOSE     ***********************  //
    virtual void close() = 0;
};

} // namespace qslary

#endif
// AioBuffer.h

#ifndef AIO_BUFFER_H
#define AIO_BUFFER_H

#include <ge/text/StringRef.h>

/*
 * Buffer used for asynchronous IO.
 */
class AioBuffer
{
    friend class AioServer;

public:
    AioBuffer();
    explicit AioBuffer(size_t bufferSize);
    AioBuffer(char* data, size_t dataLen);

    void appendData(const char* data, size_t dataLen);

    void extractData(char* buffer, size_t extractLen);

    size_t dataLen();
    size_t available();

private:
    bool seekUntil(StringRef str);

    void extract

    char* _data;
    size_t _dataSize;
    size_t _dataFilled;
    size_t _searchIndex;
};

#endif // AIO_BUFFER_H

// AioInput.h

#ifndef AIO_FILE_H
#define AIO_FILE_H

#include <ge/common.h>

/*
 * Abstract base class for some object that can read data into an AioBuffer
 * without blocking.
 */
class AioInput
{
public:
    /*
     * Reads available data and places it in the target buffer.
     *
     * Returns the number of bytes read, possibly 0, or -1 on end of stream.
     */
    ssize_t read(AioBuffer* aioBuffer) = 0;

    /*
     * Reads available data and places it in the target buffer. Will not read
     * more than maxRead bytes.
     *
     * Returns the number of bytes read, possibly 0, or -1 on end of stream.
     */
    ssize_t read(AioBuffer* aioBuffer, size_t maxRead) = 0;

private:
    AioInput(const AioInput& other) DELETED;
    AioInput& operator=(const AioInput& other) DELETED;
};

#endif // AIO_FILE_H

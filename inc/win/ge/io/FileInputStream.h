// FileInputStream.h

#ifndef FILE_INPUT_STREAM_H
#define FILE_INPUT_STREAM_H

#include <ge/common.h>
#include <ge/io/InputStream.h>
#include <ge/text/String.h>
#include <ge/thread/Mutex.h>

/*
 * Class that wraps a file handle for reading binary data. The input is
 * unbuffered except for OS level buffering. 
 *
 * Not thread safe.
 */
class EXPORT FileInputStream : public InputStream
{
friend class Process;

public:
    FileInputStream();
    ~FileInputStream();

    FileInputStream(FileInputStream&& other);
    FileInputStream& operator=(FileInputStream&& other);

    void open(const String fileName);

    uint32 available() OVERRIDE;
    void close() OVERRIDE;
    int32 read() OVERRIDE;
    int64 read(void* buffer, uint32 len) OVERRIDE;

private:
    explicit FileInputStream(HANDLE handle);
    int64 internalRead(int8* buffer, uint32 len);

    FileInputStream& operator=(const FileInputStream &other) DELETED ;
    FileInputStream(const FileInputStream& other) DELETED;

private:
    HANDLE m_handle;
};

#endif // FILE_INPUT_STREAM_H

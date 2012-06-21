// FileOutputStream.h

#ifndef FILE_OUTPUT_STREAM_H
#define FILE_OUTPUT_STREAM_H

#include <ge/common.h>
#include <ge/io/OutputStream.h>
#include <ge/text/String.h>
#include <ge/thread/Mutex.h>

/*
 * Class that wraps a file handle for writing binary data. Output is
 * unbuffered except for OS level buffering. 
 *
 * Optionally allows a file to be openned in append mode. The appends
 * will be atomic for multiple processes unless the file is across a
 * network file system (NFS) which does not support atomic appending.
 *
 * Not thread safe.
 */
class EXPORT FileOutputStream : public OutputStream
{
friend class Process;

public:
    FileOutputStream();
    ~FileOutputStream();

    FileOutputStream(FileOutputStream&& other);
    FileOutputStream& operator=(FileOutputStream&& other);

    void open(const String fileName);
    void open(const String fileName, bool append);

    void close() OVERRIDE;
    int32 write(int32 character) OVERRIDE;
    int64 write(const void* buffer, uint32 maxlen) OVERRIDE;

private:
    explicit FileOutputStream(HANDLE handle);
    void init(const String fileName, bool append);
    int64 writeNormal(const void* buffer, uint32 maxlen);
    int64 writeAppend(const void* buffer, uint32 maxlen);

    FileOutputStream& operator=(const FileOutputStream& other) DELETED;
    FileOutputStream(const FileOutputStream& other) DELETED;

private:
    HANDLE m_handle;
    bool m_append;
    OVERLAPPED m_overlapped;
};

#endif // FILE_OUTPUT_STREAM_H

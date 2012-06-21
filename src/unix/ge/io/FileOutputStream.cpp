// FileOutputStream.cpp

// Indicate to Linux headers that we can support 64 bit file offsets
#define _FILE_OFFSET_BITS 64

#include "ge/io/FileOutputStream.h"

#include "ge/io/IOException.h"
#include "ge/util/Locker.h"
#include "gepriv/UnixUtil.h"

#include <errno.h> // For error defines
#include <fcntl.h> // For create flags


FileOutputStream::FileOutputStream()
{
    m_fileDescriptor = -1;
}

FileOutputStream::FileOutputStream(int32 fileDescriptor)
{
    m_fileDescriptor = fileDescriptor;
}

FileOutputStream::~FileOutputStream()
{
    close();
}

void FileOutputStream::open(const String fileName)
{
    open(fileName, false);
}

void FileOutputStream::open(const String fileName, bool append)
{
    if (m_fileDescriptor != -1)
    {
        throw IOException("File alread open");
    }

    // Write access, create if needed, don't inherit
    int flags = O_WRONLY | O_CREAT | O_CLOEXEC;

    // If passed in append, mask on the append option,
    // otherwise truncate the file
    if (append)
    {
        flags |= O_APPEND;
    }
    else
    {
        flags |= O_TRUNC;
    }

    m_fileDescriptor = UnixUtil::sys_open(fileName.c_str(), // File name
                                          flags, // Creation flags, see above
                                          0666); // File mask if created

    if (m_fileDescriptor == -1)
    {
        throw IOException(String("open() call failed with: ") +
            UnixUtil::getLastErrorMessage());
    }
}

void FileOutputStream::close()
{
    if (m_fileDescriptor != -1)
    {
        ::close(m_fileDescriptor);
        m_fileDescriptor = -1;
    }
}

int32 FileOutputStream::write(int32 byte)
{
    char tempBuffer[1];
    tempBuffer[0] = (char)(byte & 0x000000ff);

    return (int32)internalWrite(tempBuffer, 1);
}

int64 FileOutputStream::write(const void* buffer, uint32 maxlen)
{
    return internalWrite(buffer, maxlen);
}

int64 FileOutputStream::internalWrite(const void* buffer, uint32 maxlen)
{
    if (m_fileDescriptor == -1)
    {
        throw IOException("Failed to write to stream: Stream is closed");
    }

    errno = 0;
    ssize_t bytesWritten = UnixUtil::sys_write(m_fileDescriptor, buffer, maxlen);

    // If we failed to write anything, check for closed pipe
    if (bytesWritten == -1)
    {
        uint32 lastError = errno;
        if (lastError == 0 ||
            lastError == EPIPE)
        {
            return -1;
        }
        else
        {
            throw IOException(String("write() call failed with: ") +
                UnixUtil::getLastErrorMessage());
        }
    }
    return bytesWritten;
}

// FileInputStream.cpp

// Indicate to Linux headers that we can support 64 bit file offsets
#define _FILE_OFFSET_BITS 64

#include "ge/io/FileInputStream.h"

#include "ge/io/IOException.h"
#include "ge/util/Locker.h"
#include "gepriv/UnixUtil.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

// Where FIONREAD is defined varies
//#ifdef __CYGWIN__
//#	include <sys/socket.h>
//#else
//#	include <sys/filio.h>
//#endif


FileInputStream::FileInputStream()
{
    m_fileDescriptor = -1;
}

FileInputStream::FileInputStream(int32 fileDescriptor)
{
    m_fileDescriptor = fileDescriptor;
}

FileInputStream::~FileInputStream()
{
    close();
}

uint32 FileInputStream::available()
{
    int ret = -1;

    int res = ::ioctl(m_fileDescriptor, FIONREAD, &ret);

    if (res == -1)
    {
        // TODO: Throw
    }

    if (ret < 0)
        return 0;

    return ret;
}

void FileInputStream::open(const String fileName)
{
    if (m_fileDescriptor != -1)
    {
        throw IOException("File alread open");
    }

    // Read access, create if needed, don't inherit
    int flags = O_RDONLY | O_CREAT | O_CLOEXEC;

    m_fileDescriptor = UnixUtil::sys_open(fileName.c_str(), // Name of file
                                          flags, // Creation flags, see above
                                          0666); // Permissions mask if created

    if (m_fileDescriptor == -1)
    {
        Error error = UnixUtil::getError(errno,
                                         "open",
                                         "FileInputStream::open");
        throw IOException(error);
    }
}

void FileInputStream::close()
{
    if (m_fileDescriptor != -1)
    {
        ::close(m_fileDescriptor);
        m_fileDescriptor = -1;
    }
}

int32 FileInputStream::read()
{
    char tempBuffer[1];
    int64 ret = internalRead(tempBuffer, 1);

    if (ret == -1)
        return -1;
    return tempBuffer[0];
}

int64 FileInputStream::read(void* buffer, uint32 len)
{
    return internalRead(buffer, len);
}

int64 FileInputStream::internalRead(void* buffer, uint32 len)
{
    if (m_fileDescriptor == -1)
    {
        throw IOException("Stream is closed");
    }

    //size_t bytesAvail = 0;

    // TODO: Should be removable, recheck

    // If you read from a redirected std handle of a child process read will
    // block forever if it is called after the input is complete. However, ioctl
    // correctly returns EPIPE, so we call it to check for validity.
    //
    // An alternative to this method is to use non-blocking reads, polling until
    // data is returned or waitpid indicates the process is dead.

    // FIONREAD asks how many bytes are available. We ignore the result.
    //if (::ioctl(m_fileDescriptor, FIONREAD, &bytesAvail) == -1)
    //{
    //    if (errno == EPIPE)
    //        return -1;
    //}

    errno = 0;
    ssize_t bytesRead = UnixUtil::sys_read(m_fileDescriptor, buffer, (size_t)len);

    // If we failed to read anything, check for closed pipe
    if (bytesRead == -1)
    {
        uint32 lastError = errno;
        if (lastError == 0 ||
            lastError == EPIPE)
        {
            return -1;
        }
        else
        {
            Error error = UnixUtil::getError(errno,
                                             "read",
                                             "FileOutputStream::read");
            throw IOException(error);
        }
    }

    // Some unix variants just keep returning 0 at end of pipe
    if (bytesRead == 0)
        return -1;

    return bytesRead;
}

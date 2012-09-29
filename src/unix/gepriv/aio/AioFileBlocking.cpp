// AioFileBlocking.cpp

#ifndef __linux__

// Indicate to Linux headers that we can support 64 bit file offsets
#define _FILE_OFFSET_BITS 64

#include "gepriv/aio/AioFileBlocking.h"

#include "ge/data/ShortList.h"
#include "ge/io/IOException.h"
#include "gepriv/UnixUtil.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


AioFile::AioFile() :
    _fd(-1),
    _owner(NULL)
{
}

AioFile::~AioFile()
{
    if (_owner != NULL)
    {
        // TODO: Log?
        _owner->dropFile(this);
    }

    close();
}

void AioFile::open(StringRef fileName, OpenMode_Enum mode, int permissions)
{
    // TODO: Properly convert filename
    size_t fileNameLen = fileName.length();

    ShortList<char, 256> charBuf(fileNameLen+1);

    charBuf.addBlockBack(fileName.data(), fileNameLen);
    charBuf.addBack('\0');

    // Don't inherit any fd
    int flags = O_CLOEXEC;

    // Build permission flags
    // It's legal, if normally pointless, to have no read or write access.
    // You can still access the file length.
    if ((permissions & IO_READ_ACCESS) &&
        (permissions & IO_WRITE_ACCESS))
    {
        flags |= O_RDWR;
    }
    else if (permissions & IO_READ_ACCESS)
    {
        flags |= O_WRONLY;
    }
    else if (permissions & IO_WRITE_ACCESS)
    {
        flags |= O_RDONLY;
    }

    // Build file creation flags
    switch (mode)
    {
    case OPEN_MODE_CREATE_ONLY:
        flags |= O_CREAT;
        flags |= O_EXCL;
        break;
    case OPEN_MODE_CREATE_OR_OPEN:
        flags |= O_CREAT;
        break;
    case OPEN_MODE_CREATE_OR_TRUNCATE:
        flags |= O_CREAT;
        flags |= O_TRUNC;
        break;
    case OPEN_MODE_OPEN_ONLY:
        // No extra flags
        break;
    case OPEN_MODE_TRUNCATE_ONLY:
        flags |= O_TRUNC;
        break;
    }

    _fd = UnixUtil::sys_open(charBuf.data(), flags, 0666);

    if (_fd == -1)
    {
        Error error = UnixUtil::getError(errno,
                                         "open",
                                         "AioFile::open");
        throw IOException(error);
    }
}

void AioFile::close()
{
    if (_fd != -1)
    {
        ::close(_fd);
    }

    _fd = -1;
}

#endif // !__linux__

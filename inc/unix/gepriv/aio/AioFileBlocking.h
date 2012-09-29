// AioFileBlocking.h

#ifndef AIO_FILE_BLOCKING_H
#define AIO_FILE_BLOCKING_H

#ifndef __linux__

#include <ge/common.h>
#include <ge/Error.h>
#include <ge/io/IO.h>
#include <ge/text/StringRef.h>
#include <gepriv/aio/FileServiceBlocking.h>

class FileService;

/*
 * Represents a file opened for asynchronous IO.
 */
class AioFile
{
    friend class FileService;

public:
    AioFile();
    ~AioFile();

    void open(StringRef fileName, OpenMode_Enum mode, int permissions);
    void close();

private:
    AioFile(const AioFile& other) DELETED;
    AioFile& operator=(const AioFile& other) DELETED;

    int _fd;
    FileService* _owner;
};

#endif // !__linux__

#endif // AIO_FILE_BLOCKING_H

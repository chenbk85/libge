//AioFile.h

#ifndef AIO_FILE_H
#define AIO_FILE_H

#include <ge/common.h>
#include <ge/Error.h>
#include <ge/io/IO.h>
#include <ge/text/StringRef.h>

class AioServer;

/*
 * Represents a file opened for asynchronous IO.
 */
class AioFile
{
    friend class AioServer;

public:
    AioFile();
    ~AioFile();

    void open(StringRef fileName, OpenMode_Enum mode, int permissions);
    void close();

private:
    AioFile(const AioFile& other) DELETED;
    AioFile& operator=(const AioFile& other) DELETED;

    int _fd;
    AioServer* _owner;
};

#endif // AIO_FILE_H

//AioFile.h

#ifndef AIO_FILE_H
#define AIO_FILE_H

#include <ge/common.h>
#include <ge/Error.h>
#include <ge/io/IO.h>
#include <ge/text/StringRef.h>

#define _WINSOCKAPI_
#include <Windows.h>

/*
 * Represents a file opened for asynchronous IO.
 */
class AioFile
{
    friend class AioServer;

public:
    AioFile();
    ~AioFile();

    void open(const StringRef fileName, OpenMode_Enum mode, int permissions);
    void close();

private:
    AioFile(const AioFile& other) DELETED;
    AioFile& operator=(const AioFile& other) DELETED;

    HANDLE _handle;
    AioServer* _owner;
};

#endif // AIO_FILE_H

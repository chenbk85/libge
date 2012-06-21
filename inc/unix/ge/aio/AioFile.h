//AioFile.h

#ifndef AIO_FILE_H
#define AIO_FILE_H

#include <ge/common.h>
#include <ge/io/IO.h>

/*
 * Represents a file opened for asynchronous IO.
 */
class AioFile
{
public:
    AioFile();
    ~AioFile();

    void open(StringRef fileName, OpenMode_Enum mode, int permissions);
    void close();

private:
    AioFile(const AioFile& other) DELETED;
    AioFile& operator=(const AioFile& other) DELETED;

    int m_fd;
};

#endif // AIO_FILE_H

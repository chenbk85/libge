// FileServiceLinuxAio.h

#ifndef FILE_SERVICE_LINUX_AIO_H
#define FILE_SERVICE_LINUX_AIO_H

#ifdef __linux__

#include <gepriv/aio/FileService.h>

/*
 * FileService implementation that uses blocking IO
 */
class FileServiceLinuxAio : public FileService
{
public:
    FileServiceLinuxAio();
    ~FileServiceLinuxAio();

    /*
     * Triggers the blocking IO.
     */
    void process() OVERRIDE;

    void submitRead(AioFile* aioFile,
                    AioServer::fileCallback callback,
                    void* userData,
                    uint64 pos,
                    char* buffer,
                    uint32 bufferLen) OVERRIDE;

    void submitWrite(AioFile* aioFile,
                     AioServer::fileCallback callback,
                     void* userData,
                     uint64 pos,
                     const char* buffer,
                     uint32 bufferLen) OVERRIDE;

private:
    FileServiceLinuxAio(const FileServiceLinuxAio&) DELETED;
    FileServiceLinuxAio& operator=(const FileServiceLinuxAio&) DELETED;
};

#endif // #ifdef __linux__

#endif // FILE_SERVICE_LINUX_AIO_H

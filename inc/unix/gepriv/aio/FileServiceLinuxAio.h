// FileServiceLinuxAio.h

#ifndef FILE_SERVICE_LINUX_AIO_H
#define FILE_SERVICE_LINUX_AIO_H

#ifdef __linux__

#include <ge/aio/AioServer.h>
#include <gepriv/aio/FileService.h>

/*
 * FileService implementation that uses Linux file aio.
 */
class FileServiceLinuxAio : public FileService
{
public:
    FileServiceLinuxAio(AioServer aioServer);
    ~FileServiceLinuxAio();

    void process() OVERRIDE;

    void shutdown() OVERRIDE;

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

    AioServer _aioServer;
};

#endif // #ifdef __linux__

#endif // FILE_SERVICE_LINUX_AIO_H

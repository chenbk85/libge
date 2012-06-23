//AioFileService.h

#ifndef FILE_SERVICE_H
#define FILE_SERVICE_H

#include <ge/common.h>
#include <ge/aio/AioServer.h>

class AioFile;

/*
 * Abstract base class for something that provides an interface for
 * asynchronous file IO.
 */
class FileService
{
public:
    /*
     * Does whatever processing it needs to do. This will be called once per
     * time the AioServer was told data was ready. This can block if needed,
     * although it is obviously preferable not to.
     */
    virtual
    void process() = 0;

    virtual
    void submitRead(AioFile* aioFile,
                    AioServer::fileCallback callback,
                    void* userData,
                    uint64 pos,
                    char* buffer,
                    uint32 bufferLen) = 0;

    virtual
    void submitWrite(AioFile* aioFile,
                     AioServer::fileCallback callback,
                     void* userData,
                     uint64 pos,
                     const char* buffer,
                     uint32 bufferLen) = 0;

protected:
    FileService() {};

private:
    FileService(const FileService&) DELETED;
    FileService& operator=(const FileService&) DELETED;
};

#endif // FILE_SERVICE_H


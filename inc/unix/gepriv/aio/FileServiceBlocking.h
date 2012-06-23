// FileServiceBlocking.h

#ifndef FILE_SERVICE_BLOCKING_H
#define FILE_SERVICE_BLOCKING_H

#include <gepriv/aio/FileService.h>

/*
 * FileService implementation that uses blocking IO
 */
class FileServiceBlocking : public FileService
{
public:
    FileServiceBlocking();
    ~FileServiceBlocking();

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
    FileServiceBlocking(const FileServiceBlocking&) DELETED;
    FileServiceBlocking& operator=(const FileServiceBlocking&) DELETED;
};

#endif // FILE_SERVICE_BLOCKING_H

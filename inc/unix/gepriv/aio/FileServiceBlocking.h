// FileServiceBlocking.h

#ifndef FILE_SERVICE_BLOCKING_H
#define FILE_SERVICE_BLOCKING_H

#ifndef __linux__

#include <ge/data/DLinkedList.h>
#include <ge/data/List.h>
#include <ge/thread/Condition.h>
#include <ge/thread/Thread.h>
#include <gepriv/aio/AioFileBlocking.h>

class AioFile;

/*
 * FileService implementation that uses blocking IO (read, write, etc).
 */
class FileService
{
public:
    friend class AioFile;
    friend class AioWorker;

    typedef void (*fileCallback)(AioFile* aioFile,
                                 void* userData,
                                 uint32 bytesTransfered,
                                 const Error& error);

    FileService();
    ~FileService();

    void startServing(uint32 desiredThreads);
    void shutdown();

    void fileRead(AioFile* aioFile,
                  FileService::fileCallback callback,
                  void* userData,
                  uint64 pos,
                  char* buffer,
                  uint32 bufferLen);

    void fileWrite(AioFile* aioFile,
                   FileService::fileCallback callback,
                   void* userData,
                   uint64 pos,
                   const char* buffer,
                   uint32 bufferLen);

private:
    FileService(const FileService&) DELETED;
    FileService& operator=(const FileService&) DELETED;

    void dropFile(AioFile* aioFile);
    bool process();

    class AioWorker : public Thread
    {
    public:
        AioWorker(FileService* fileService);
        void run() OVERRIDE;

    private:
        FileService* _fileService;
    };

    class QueueData
    {
    public:
        AioFile* aioFile;
        bool isRead;
        FileService::fileCallback callback;
        void* userData;
        uint64 pos;
        char* buffer;
        uint32 bufferLen;
    };

    Condition _cond;               // Condition guarding data
    bool _isShutdown;              // Indicates if shutdown
    List<AioWorker*> _threads;     // List of threads created
    DLinkedList<QueueData> _queue; // Queue of operations to perform
};

#endif // ifdef __linux__

#endif // FILE_SERVICE_BLOCKING_H

// FileService.h

#ifndef FILE_SERVICE_H
#define FILE_SERVICE_H

#include <ge/common.h>
#include <ge/Error.h>
#include <ge/aio/AioFile.h>
#include <ge/data/List.h>
#include <ge/thread/Mutex.h>
#include <ge/thread/Thread.h>

class AioFile;

/*
 * Server that allows asynchronous operations on files.
 * 
 * When the server is started, it will create a set of threads for calling OS
 * asynchronous file IO functions. When the OS indicates that IO has completed,
 * a worker thread will trigger the passed user callback.
 *
 * As not every OS implements non-blocking file IO, and some operations such
 * as file appends tend to require blocking, some worker threads may be forced
 * to block. Make sure to adjust the pool size appropriately.
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

    class AioWorker : public Thread
    {
    public:
        AioWorker(FileService* fileService);
        void run() OVERRIDE;

    private:
        FileService* _fileService;
    };

    Error addFile(AioFile* aioFile,
                  const char* context);
    void dropFile(AioFile* aioFile);

    bool process();


    HANDLE _completionPort;      // IO completion port
    List<AioWorker*> _threads;   // List of threads created

    LONG volatile _state;        // Current server state (1 = started, 2 = shutdown)
    LONG volatile _pending;      // Number of pending IO requests

    Mutex _lock;                 // Lock for set of files and sockets
    List<AioFile*> _files;       // Set of files
};


#endif // FILE_SERVICE_H

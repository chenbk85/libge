// AioServer.h

#ifndef AIO_SERVER_H
#define AIO_SERVER_H

#include <ge/common.h>
#include <ge/Error.h>
#include <ge/aio/AioFile.h>
#include <ge/aio/AioSocket.h>
#include <ge/data/List.h>
#include <ge/thread/Mutex.h>

/*
 * Server that allows asynchronous operations on files and sockets.
 * 
 * When the server is started, it will create a set of threads for calling OS
 * polling functions (poll, epoll_wait, etc) and a set of worker threads for
 * doing processing.
 *
 * When the OS indicates that IO has completed, a worker thread will trigger
 * the passed user callback.
 *
 * Note that some systems do not support all asynchronous operations. These
 * will be emulated by having a worker thread call an equivalent blocking
 * function.
 *
 * Even if the system does support asynchronous IO, some normally asynchronous
 * operations can block in some situations (extending a file, for example).
 * You need to tune the number of worker threads to account for the time
 * spent in callbacks and the expected level of blocking.
 */
class AioServer
{
    friend class AioFile;
    friend class AioSocket;
    friend class AioWorker;

public:
    typedef void (*fileCallback)(AioFile* aioFile,
                                 void* userData,
                                 uint32 bytesTransfered,
                                 const Error& error);
    typedef void (*socketCallback)(AioSocket* aioSocket,
                                   void* userData,
                                   uint32 bytesTransfered,
                                   const Error& error);
    typedef void (*acceptCallback)(AioSocket* aioSocket,
                                   AioSocket* acceptedSocket,
                                   void* userData,
                                   const Error& error);
    typedef void (*connectCallback)(AioSocket* aioSocket,
                                    void* userData,
                                    const Error& error);

    AioServer();
    ~AioServer();

    void startServing(uint32 desiredThreads);

    void shutdown();

    void fileRead(AioFile* aioFile,
                  fileCallback callback,
                  void* userData,
                  uint64 pos,
                  char* buffer,
                  uint32 bufferLen);

    void fileWrite(AioFile* aioFile,
                   fileCallback callback,
                   void* userData,
                   uint64 pos,
                   const char* buffer,
                   uint32 bufferLen);

    void socketClose(AioSocket* aioSocket,
                     connectCallback callback,
                     void* userData);

    void socketAccept(AioSocket* listenSocket,
                      AioSocket* acceptSocket,
                      acceptCallback callback,
                      void* userData);

    void socketConnect(AioSocket* aioSocket,
                       connectCallback callback,
                       void* userData,
                       const INetAddress& address,
                       int32 port);

    void socketConnect(AioSocket* aioSocket,
                       connectCallback callback,
                       void* userData,
                       const INetAddress& address,
                       int32 port,
                       int32 timeout);

    void socketRead(AioSocket* aioSocket,
                    socketCallback callback,
                    void* userData,
                    char* buffer,
                    uint32 bufferLen);

    void socketWrite(AioSocket* aioSocket,
                     socketCallback callback,
                     void* userData,
                     const char* buffer,
                     uint32 bufferLen);

    void socketSendFile(AioSocket* aioSocket,
                        socketCallback callback,
                        void* userData,
                        AioFile* aioFile,
                        uint64 pos,
                        uint32 writeLen);

private:
    Error addFile(AioFile* aioFile,
                  const char* context);
    Error addSocket(AioSocket* aioSocket,
                    const char* context);

    void dropFile(AioFile* aioFile);
    void dropSocket(AioSocket* aioSocket);

    bool process();


    HANDLE _completionPort;      // IO completion port
    List<AioWorker*> _threads;   // List of threads created

    LONG volatile _state;        // Current server state (1 = started, 2 = shutdown)
    LONG volatile _pending;      // Number of pending IO requests

    Mutex _lock;                 // Lock for set of files and sockets
    List<AioFile*> _files;       // Set of files
    List<AioSocket*> _sockets;   // Set of sockets
};

#endif // AIO_SERVER_H

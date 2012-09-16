// SocketService.h

#ifndef SOCKET_SERVICE_H
#define SOCKET_SERVICE_H

#include <ge/common.h>
#include <ge/Error.h>
#include <ge/aio/AioFile.h>
#include <ge/aio/AioSocket.h>
#include <ge/data/List.h>
#include <ge/thread/Mutex.h>
#include <ge/thread/Thread.h>


class AioFile;
class AioSocket;

/*
 * Server that allows asynchronous operations on sockets.
 * 
 * When the server is started, it will create a set of threads for calling OS
 * asynchronous file IO functions. When the OS indicates that IO has completed,
 * a worker thread will trigger the passed user callback.
 *
 * Note that on some systems the sendfile functionality may be emulated using
 * blocking io.
 */
class SocketService
{
public:
    friend class AioSocket;
    friend class AioWorker;
    friend class PollWorker;

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

    SocketService();
    ~SocketService();

    void startServing(uint32 desiredThreads);
    void shutdown();

    void socketAccept(AioSocket* listenSocket,
                      AioSocket* acceptSocket,
                      SocketService::acceptCallback callback,
                      void* userData);

    void socketConnect(AioSocket* aioSocket,
                       SocketService::connectCallback callback,
                       void* userData,
                       const INetAddress& address,
                       int32 port);

    void socketRead(AioSocket* aioSocket,
                    SocketService::socketCallback callback,
                    void* userData,
                    char* buffer,
                    uint32 bufferLen);

    void socketWrite(AioSocket* aioSocket,
                     SocketService::socketCallback callback,
                     void* userData,
                     const char* buffer,
                     uint32 bufferLen);

    void socketSendFile(AioSocket* aioSocket,
                        SocketService::socketCallback callback,
                        void* userData,
                        AioFile* aioFile,
                        uint64 pos,
                        uint32 writeLen);

private:
    SocketService(const SocketService&) DELETED;
    SocketService& operator=(const SocketService&) DELETED;

    class AioWorker : public Thread
    {
    public:
        AioWorker(SocketService* socketService);
        void run() OVERRIDE;

    private:
        SocketService* _socketService;
    };


    Error addSocket(AioSocket* aioSocket,
                    const char* context);
    void dropSocket(AioSocket* aioSocket);

    bool process();


    HANDLE _completionPort;      // IO completion port
    List<AioWorker*> _threads;   // List of threads created

    LONG volatile _state;        // Current server state (1 = started, 2 = shutdown)
    LONG volatile _pending;      // Number of pending IO requests

    Mutex _lock;                 // Lock for set of files and sockets
    List<AioSocket*> _sockets;   // Set of sockets
};

#endif // SOCKET_SERVICE_H

// SocketServicePoll.h

#ifndef SOCKET_SERVICE_POLL_H
#define SOCKET_SERVICE_POLL_H

#include <ge/Error.h>
#include <ge/aio/AioFile.h>
#include <ge/aio/AioSocket.h>
#include <ge/data/HashMap.h>
#include <ge/data/List.h>
#include <ge/inet/INetAddress.h>
#include <ge/thread/Condition.h>
#include <ge/thread/Thread.h>

#include <poll.h>

class AioFile;
class AioSocket;

/*
 * SocketService implementation that uses the poll() system call.
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

    class PollWorker : public Thread
    {
    public:
        PollWorker(SocketService* socketService);
        void run() OVERRIDE;

    private:
        SocketService* _socketService;
    };

    class SockData;

    class QueueEntry
    {
    public:
        bool isRead;
        SockData* data;
        QueueEntry* next;
        QueueEntry* prev;
    };

    class SockData
    {
    public:
        QueueEntry readQueueEntry;
        QueueEntry writeQueueEntry;

        AioSocket* aioSocket;
        uint32 operMask;

        // Read data
        uint32 readOper;
        AioSocket* acceptSocket;
        void* readCallback;
        void* readUserData;
        char* readBuffer;
        uint32 readBufferPos;
        uint32 readBufferLen;

        // Write data
        uint32 writeOper;
        void* writeCallback;
        void* writeUserData;
        char* writeBuffer;
        uint32 writeBufferPos;
        uint32 writeBufferLen;

        INetAddress connectAddress;
        int32 connectPort;

        int sendFileFd;
        char* sendFileBuf;
        uint32 sendFileBufFilled;
        uint32 sendFileBufIndex;
        uint64 sendFileOffset;
        uint64 sendFileEnd;

        SockData();
        ~SockData();
    };

    void emptyWakePipe();
    void wakeup();

    void dropSocket(AioSocket* aioSocket);
    bool process();
    bool poll();

    void enqueData(QueueEntry* queueEntry);

    bool handleAcceptReady(SockData* sockData, Error& error);
    bool handleConnectReady(SockData* sockData, Error& error);
    bool handleReadReady(SockData* sockData, Error& error);
    bool handleWriteReady(SockData* sockData, Error& error);
    bool handleSendfileReady(SockData* sockData, Error& error);


    int _wakeupPipe[2];

    Condition _cond;

    List<AioWorker*> _threads;
    PollWorker _pollWorker;

    bool _isShutdown;
    List<pollfd> _pollfdList;
    HashMap<int, SockData> _dataMap;
    QueueEntry* _readyQueueHead;
    QueueEntry* _readyQueueTail;
};

#endif // SOCKET_SERVICE_POLL_H

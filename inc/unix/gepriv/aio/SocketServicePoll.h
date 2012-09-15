// SocketServicePoll.h

#ifndef SOCKET_SERVICE_POLL_H
#define SOCKET_SERVICE_POLL_H

#include <ge/aio/AioServer.h>
#include <ge/data/HashMap.h>
#include <ge/data/List.h>
#include <ge/thread/Condition.h>
#include <gepriv/aio/SocketService.h>

#include <poll.h>

/*
 * SocketService implementation that uses the poll() system call.
 */
class SocketServicePoll : public SocketService
{
public:
    SocketServicePoll(AioServer* aioServer);
    ~SocketServicePoll();

    /*
     * Calls read(), write() on ready sockets and triggers callbacks.
     */
    void process() OVERRIDE;

    void shutdown() OVERRIDE;

    void submitAccept(AioSocket* listenSocket,
                      AioSocket* acceptSocket,
                      AioServer::acceptCallback callback,
                      void* userData) OVERRIDE;

    void submitConnect(AioSocket* aioSocket,
                       AioServer::connectCallback callback,
                       void* userData,
                       const INetAddress& address,
                       int32 port) OVERRIDE;

    void submitConnect(AioSocket* aioSocket,
                       AioServer::connectCallback callback,
                       void* userData,
                       const INetAddress& address,
                       int32 port,
                       int32 timeout) OVERRIDE;

    void socketRead(AioSocket* aioSocket,
                    AioServer::socketCallback callback,
                    void* userData,
                    char* buffer,
                    uint32 bufferLen) OVERRIDE;

    void socketWrite(AioSocket* aioSocket,
                     AioServer::socketCallback callback,
                     void* userData,
                     const char* buffer,
                     uint32 bufferLen) OVERRIDE;

    void socketSendFile(AioSocket* aioSocket,
                        AioServer::socketCallback callback,
                        void* userData,
                        AioFile* aioFile,
                        uint64 pos,
                        uint32 writeLen) OVERRIDE;

private:
    SocketServicePoll(const SocketServicePoll&) DELETED;
    SocketServicePoll& operator=(const SocketServicePoll&) DELETED;

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

    void enqueData(QueueEntry* queueEntry);

    static void* pollFunc(void* threadData);

    bool handleAcceptReady(SockData* sockData, Error& error);
    bool handleConnectReady(SockData* sockData, Error& error);
    bool handleReadReady(SockData* sockData, Error& error);
    bool handleWriteReady(SockData* sockData, Error& error);
    bool handleSendfileReady(SockData* sockData, Error& error);


    AioServer* _aioServer;

    pthread_t _pollThread;
    int _wakeupPipe[2];

    Condition _cond;
    bool _shutdown;
    List<pollfd> _pollfdList;
    HashMap<int, SockData> _dataMap;
    QueueEntry* _readyQueueHead;
    QueueEntry* _readyQueueTail;
};

#endif // SOCKET_SERVICE_POLL_H

// SocketServicePoll.h

#ifndef SOCKET_SERVICE_POLL_H
#define SOCKET_SERVICE_POLL_H

#include <ge/aio/AioServer.h>
#include <ge/data/HashMap.h>
#include <ge/data/List.h>
#include <ge/data/DLinkedList.h>
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

    void emptyWakePipe();
    void wakeup();

    struct SocketOper
    {
        AioSocket* aioSocket;
        uint32 operType;
        AioSocket* acceptSocket;
        void* callback;
        void* userData;
        char* buffer;
        uint32 bufferPos;
        uint32 bufferLen;
    };

    struct SockData
    {
        SocketOper readOper;
        SocketOper writeOper;
        char* sendFileBuf;
        uint32 operMask;
    };

    AioServer* _aioServer;

    int _wakeupPipe[2];

    Condition _cond;
    DLinkedList<SocketOper*> _operQueue;

    Mutex _lock;
    List<pollfd> _pollfdList;
    HashMap<int, SockData> _dataMap;
};

#endif // SOCKET_SERVICE_POLL_H

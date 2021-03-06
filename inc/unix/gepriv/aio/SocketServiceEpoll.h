// SocketServiceEpoll.h

#ifdef __linux__

#ifndef SOCKET_SERVICE_EPOLL_H
#define SOCKET_SERVICE_EPOLL_H

#include <ge/aio/AioServer.h>
#include <gepriv/aio/SocketServiceEpoll.h>


/*
 * SocketService implementation that uses the epoll() system calls.
 */
class SocketServiceEpoll : public SocketService
{
public:
    SocketServiceEpoll(AioServer* aioServer);
    ~SocketServiceEpoll();

    /*
     * Calls read(), write() on ready sockets and triggers callbacks.
     */
    void process() OVERRIDE;

    void shutdown() OVERRIDE;

    void submitClose(AioSocket* aioSocket,
                     AioServer::connectCallback callback,
                     void* userData) OVERRIDE;

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
    SocketServiceEpoll(const SocketServiceEpoll&) DELETED;
    SocketServiceEpoll& operator=(const SocketServiceEpoll&) DELETED;

    AioServer* _aioServer;
};

#endif // SOCKET_SERVICE_EPOLL_H

#endif // #ifdef __linux__

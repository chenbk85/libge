// SocketServicePoll.h

#ifndef SOCKET_SERVICE_POLL_H
#define SOCKET_SERVICE_POLL_H

#include <gepriv/aio/SocketService.h>

/*
 * SocketService implementation that uses the poll() system call.
 */
class SocketServicePoll : public SocketService
{
public:
    SocketServicePoll();
    ~SocketServicePoll();

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
    SocketServicePoll(const SocketServicePoll&) DELETED;
    SocketServicePoll& operator=(const SocketServicePoll&) DELETED;
};

#endif // SOCKET_SERVICE_POLL_H

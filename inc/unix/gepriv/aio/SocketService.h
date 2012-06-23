// SocketService.h

#ifndef SOCKET_SERVICE_H
#define SOCKET_SERVICE_H

#include <ge/common.h>
#include <ge/aio/AioServer.h>

class AioSocket;

/*
 * Abstract base class for something that provides an interface for
 * asynchronous socket IO.
 */
class SocketService
{
public:
    /*
     * Does whatever processing it needs to do. This will be called once per
     * time the AioServer was told data was ready.  This can block if needed,
     * although it is obviously preferable not to.
     */
    virtual
    void process() = 0;

    virtual
    void submitClose(AioSocket* aioSocket,
                     AioServer::connectCallback callback,
                     void* userData) = 0;

    virtual
    void submitAccept(AioSocket* listenSocket,
                      AioSocket* acceptSocket,
                      AioServer::acceptCallback callback,
                      void* userData) = 0;

    virtual
    void submitConnect(AioSocket* aioSocket,
                       AioServer::connectCallback callback,
                       void* userData,
                       const INetAddress& address,
                       int32 port) = 0;

    virtual
    void submitConnect(AioSocket* aioSocket,
                       AioServer::connectCallback callback,
                       void* userData,
                       const INetAddress& address,
                       int32 port,
                       int32 timeout) = 0;

    virtual
    void socketRead(AioSocket* aioSocket,
                    AioServer::socketCallback callback,
                    void* userData,
                    char* buffer,
                    uint32 bufferLen) = 0;

    virtual
    void socketWrite(AioSocket* aioSocket,
                     AioServer::socketCallback callback,
                     void* userData,
                     const char* buffer,
                     uint32 bufferLen) = 0;

    virtual
    void socketSendFile(AioSocket* aioSocket,
                        AioServer::socketCallback callback,
                        void* userData,
                        AioFile* aioFile,
                        uint64 pos,
                        uint32 writeLen) = 0;

protected:
    SocketService() {};

private:
    SocketService(const SocketService&) DELETED;
    SocketService& operator=(const SocketService&) DELETED;
};

#endif // SOCKET_SERVICE_H

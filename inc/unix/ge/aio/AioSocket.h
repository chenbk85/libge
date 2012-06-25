// AioSocket.h

#ifndef AIO_SOCKET_H
#define AIO_SOCKET_H

#include <ge/common.h>
#include <ge/inet/INet.h>
#include <ge/inet/INetAddress.h>

class AioServer;

// TODO: Need access to source and dest address

class AioSocket
{
    friend class AioServer;
    friend class SocketServicePoll;
    friend class SocketServiceEpoll;

public:
    AioSocket();
    ~AioSocket();

// TODO: This needs to move in owning server
    AioSocket(AioSocket&& other);
    AioSocket& operator=(AioSocket&& other);

    void init(INetProt_Enum family);
    void hardClose();

    void listen();
    void listen(int32 backlog);

    void bind(const INetAddress& address, int32 port);

private:
    AioSocket(const AioSocket& other) DELETED;
    AioSocket& operator=(const AioSocket& other) DELETED;

    INetProt_Enum _family;
    int _sockFd;
    AioServer* _owner;

    INetAddress _localAddress;
    uint32 _localPort;
    INetAddress _remoteAddress;
    uint32 _remotePort;

    int _flags;
};

#endif // AIO_SOCKET_H

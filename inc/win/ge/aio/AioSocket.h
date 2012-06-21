// AioSocket.h

#ifndef AIO_SOCKET_H
#define AIO_SOCKET_H

#include <ge/common.h>
#include <ge/inet/INet.h>
#include <ge/inet/INetAddress.h>

#include <WinSock2.h>

class AioSocket
{
    friend class AioServer;

public:
    AioSocket();
    ~AioSocket();

#if defined(HAVE_RVALUE)
    AioSocket(AioSocket&& other);
    AioSocket& operator=(AioSocket&& other);
#endif

    void init(INetProt_Enum family);
    void hardClose();

    void listen();
    void listen(int32 backlog);

    void bind(const INetAddress& address, int32 port);

private:
    AioSocket(const AioSocket& other) DELETED;
    AioSocket& operator=(const AioSocket& other) DELETED;

    INetProt_Enum _family;
    SOCKET _winSocket;
    AioServer* _owner;

    INetAddress _localAddress;
    uint32 _localPort;
    INetAddress _remoteAddress;
    uint32 _remotePort;

    int _flags;
};

#endif // AIO_SOCKET_H

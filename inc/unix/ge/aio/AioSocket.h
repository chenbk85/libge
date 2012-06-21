// AioSocket.h

#ifndef AIO_SOCKET_H
#define AIO_SOCKET_H

#include <ge/common.h>

class AioSocket
{
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
    void listen(int backlog);

    void accept(Socket* clientSocket);

    void bind(const INetAddress& address, int port);

    void connect(const INetAddress& address, int port);
    void connect(const INetAddress& address, int port, int timeout);

    void getConnectionAddress(INetAddress* address);

private:
    AioSocket(const AioSocket& other) DELETED;
    AioSocket& operator=(const AioSocket& other) DELETED;
};

#endif // AIO_SOCKET_H

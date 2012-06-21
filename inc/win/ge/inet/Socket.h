// Socket.h

#ifndef SOCKET_H
#define SOCKET_H

#include <ge/inet/INetAddress.h>
#include <ge/io/InputStream.h>
#include <ge/io/OutputStream.h>

#include <Winsock2.h>

/*
 * Socket object supporting both IPv4 and IPv6.
 *
 * There is no current dual stack support, so you will need to explicitly
 * use IPv4 or IPv6. If you want to perform some action on both protocols,
 * you'll need to create two sockets.
 */
class Socket
{
    friend class Selector;
    friend class SocketInputStream;
    friend class SocketOutputStream;

public:
    Socket();
    ~Socket();

#if defined(HAVE_RVALUE)
    Socket(Socket&& other);
    Socket& operator=(Socket&& other);
#endif

    void init(INetProt_Enum family);
    void close();

    void listen();
    void listen(int32 backlog);

    void accept(Socket* clientSocket);

    void bind(const INetAddress& address, int32 port);

    void connect(const INetAddress& address, int32 port);
    void connect(const INetAddress& address, int32 port, int32 timeout);

    void getConnectionAddress(INetAddress* address);

    InputStream* getInputStream();
    OutputStream* getOutputStream();

private:
    Socket(const Socket& other) DELETED;
    Socket& operator=(const Socket& other) DELETED;

    INetProt_Enum m_family;
    INetAddress m_connAddress;
    SOCKET m_winSocket;
    InputStream* m_inputStream;
    OutputStream* m_outputStream;
};

#endif // SOCKET_H

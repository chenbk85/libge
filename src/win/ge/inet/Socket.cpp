// Socket.cpp

#include "ge/inet/Socket.h"

#include "ge/inet/INetException.h"

#include "gepriv/WinUtil.h"

#include <ws2tcpip.h>

// Stream Implementations ---------------------------------------------------

/*
 * InputStream for a Windows SOCKET.
 */
class SocketInputStream : public InputStream
{
public:
    SocketInputStream(Socket* socket);
    ~SocketInputStream();

    uint32 available() OVERRIDE;
	void close() OVERRIDE;
	int32 read() OVERRIDE;
	int64 read(void *buffer, uint32 len) OVERRIDE;

private:
    SocketInputStream(const SocketInputStream&) DELETED;
    SocketInputStream& operator=(const SocketInputStream&) DELETED;

    Socket* m_parent;
};

SocketInputStream::SocketInputStream(Socket* socket) :
    m_parent(socket)
{
}

SocketInputStream::~SocketInputStream()
{
}

uint32 SocketInputStream::available()
{
    u_long ret;
    
    int ioctlRet = ::ioctlsocket(m_parent->m_winSocket, FIONREAD, &ret);

    if (ioctlRet == SOCKET_ERROR)
    {
        throw INetException(WinUtil::getWSALastErrorMessage());
    }

    return (uint32)ret;
}

void SocketInputStream::close()
{
    m_parent->close();
}

int32 SocketInputStream::read()
{
    char buffer[1];
    return (int32)read(buffer, 1);
}

int64 SocketInputStream::read(void* buffer, uint32 len)
{
    // Prevent overflow to negative
    if (len > INT_MAX)
        len = INT_MAX;

    int recvRet = ::recv(m_parent->m_winSocket, (char*)buffer, (int)len, 0);

    if (recvRet == SOCKET_ERROR)
    {
        throw INetException(WinUtil::getWSALastErrorMessage());
    }

    return recvRet;
}

/*
 * OutputStream for a Windows SOCKET.
 */
class SocketOutputStream : public OutputStream
{
public:
    SocketOutputStream(Socket* socket);
    ~SocketOutputStream();

	void close() OVERRIDE;
	int32 write(int32 byte) OVERRIDE;
	int64 write(const void* buffer, uint32 maxlen) OVERRIDE;

private:
    SocketOutputStream(const SocketOutputStream&) DELETED;
    SocketOutputStream& operator=(const SocketOutputStream&) DELETED;

    Socket* m_parent;
};

SocketOutputStream::SocketOutputStream(Socket* socket) :
    m_parent(socket)
{
}

SocketOutputStream::~SocketOutputStream()
{
}

void SocketOutputStream::close()
{
    m_parent->close();
}

int32 SocketOutputStream::write(int32 byte)
{
    char buffer[1] = {(char)byte};
    return (int32)write(buffer, 1);
}

int64 SocketOutputStream::write(const void* buffer, uint32 len)
{
    // Prevent overflow to negative
    if (len > INT_MAX)
        len = INT_MAX;

    int sendRet = ::send(m_parent->m_winSocket, (char*)buffer, (int)len, 0);

    if (sendRet == SOCKET_ERROR)
    {
        throw INetException(WinUtil::getWSALastErrorMessage());
    }

    return sendRet;
}

// Socket class implementation ----------------------------------------------

Socket::Socket() :
    m_winSocket(INVALID_SOCKET),
    m_inputStream(NULL),
    m_outputStream(NULL)
{
}

Socket::~Socket()
{
    close();
}

#if defined(HAVE_RVALUE)
Socket::Socket(Socket&& other)
{
    m_winSocket = other.m_winSocket;
    other.m_winSocket = INVALID_SOCKET;
}

Socket& Socket::operator=(Socket&& other)
{
    if (this != &other)
    {
        m_winSocket = other.m_winSocket;
        other.m_winSocket = INVALID_SOCKET;
    }

    return *this;
}
#endif

void Socket::init(INetProt_Enum family)
{
    if (m_winSocket != INVALID_SOCKET)
    {
        throw INetException("Socket already initialized");
    }

    int prot = AF_INET;

    if (family == INET_PROT_IPV6)
    {
        prot = AF_INET6;
    }
    
    m_winSocket = ::socket(prot, SOCK_STREAM, IPPROTO_TCP);

    if (m_winSocket == INVALID_SOCKET)
    {
        throw INetException(WinUtil::getWSALastErrorMessage());
    }

    m_family = family;
}

void Socket::close()
{
    char discardBuffer[512];

    if (m_winSocket != INVALID_SOCKET)
    {
        // Start shutdown
        int shutRet = ::shutdown(m_winSocket, SD_SEND);

        if (shutRet != 0)
        {
            // TODO: Log
        }

        // Receive until nothing to read or error
        int recvRet;

        do
        {
            recvRet = ::recv(m_winSocket, discardBuffer, sizeof(discardBuffer), 0);
        } while (recvRet != 0 && recvRet != SOCKET_ERROR);

        // Close the socket
        int closeRet = ::closesocket(m_winSocket);

        if (closeRet != 0)
        {
            // TODO: Log
        }

        m_winSocket = INVALID_SOCKET;
    }

    delete m_inputStream;
    m_inputStream = NULL;
    delete m_outputStream;
    m_outputStream = NULL;
}

void Socket::listen()
{
    listen(SOMAXCONN);
}

void Socket::listen(int32 backlog)
{
    if (backlog <= 0 ||
        backlog > SOMAXCONN)
    {
        backlog = SOMAXCONN;
    }

    int ret = ::listen(m_winSocket, backlog);

    if (ret)
    {
        throw INetException(WinUtil::getWSALastErrorMessage());
    }
}

void Socket::accept(Socket* clientSocket)
{
    SOCKET sock;

    if (clientSocket->m_winSocket != INVALID_SOCKET)
    {
        throw INetException("Can't accept with initialized socket");
    }

    clientSocket->init(m_family);
    
    if (m_family == INET_PROT_IPV4)
    {
        sockaddr_in ipv4Address;
        ::memset(&ipv4Address, 0, sizeof(ipv4Address));

        int32 retAddrSize = sizeof(ipv4Address);
        sock = ::accept(m_winSocket, (sockaddr*)&ipv4Address, &retAddrSize);

        if (sock == INVALID_SOCKET)
        {
            throw INetException(WinUtil::getWSALastErrorMessage());
        }

        ::memcpy(&clientSocket->m_connAddress.m_addr, &ipv4Address.sin_addr, 4);
        ::memset(&clientSocket->m_connAddress.m_addr+4, 0, 12);
    }
    else
    {
        sockaddr_in6 ipv6Address;
        ::memset(&ipv6Address, 0, sizeof(ipv6Address));

        int retAddrSize = sizeof(ipv6Address);
        sock = ::accept(m_winSocket, (sockaddr*)&ipv6Address, &retAddrSize);

        if (sock == INVALID_SOCKET)
        {
            throw INetException(WinUtil::getWSALastErrorMessage());
        }

        ::memcpy(&clientSocket->m_connAddress.m_addr, &ipv6Address.sin6_addr, 16);
    }

    clientSocket->m_winSocket = sock;
    clientSocket->m_family = m_family;
    clientSocket->m_connAddress.m_family = m_family;
}

void Socket::bind(const INetAddress& address, int32 port)
{
    INetProt_Enum family = address.getFamily();

    if (family != m_family)
    {
        if (m_family == INET_PROT_IPV4)
        {
            throw INetException("Cannot bind IPv4 socket to IPv6 address");
        }
        else
        {
            throw INetException("Cannot bind IPv6 socket to IPv4 address");
        }
    }

    const unsigned char* addrData = address.getAddrData();
    int ret = 0;

    if (family == INET_PROT_IPV4)
    {
        sockaddr_in ipv4SockAddr;
        ::memset(&ipv4SockAddr, 0, sizeof(ipv4SockAddr));

        ipv4SockAddr.sin_family = AF_INET;
        ::memcpy(&ipv4SockAddr.sin_addr.S_un, addrData, 4);
        ipv4SockAddr.sin_port = htons(port);

        ret = ::bind(m_winSocket,
                     (const sockaddr*)&ipv4SockAddr,
                     sizeof(ipv4SockAddr));
    }
    else
    {
        sockaddr_in6 ipv6SockAddr;
        ::memset(&ipv6SockAddr, 0, sizeof(ipv6SockAddr));

        ipv6SockAddr.sin6_family = AF_INET6;
        ::memcpy(&ipv6SockAddr.sin6_addr.u, addrData, 16);
        ipv6SockAddr.sin6_port = htons(port);

        ret = ::bind(m_winSocket,
                     (const sockaddr*)&ipv6SockAddr,
                     sizeof(ipv6SockAddr));
    }

    if (ret)
    {
        throw INetException(WinUtil::getWSALastErrorMessage());
    }
}

void Socket::connect(const INetAddress& address, int32 port)
{
    connect(address, port, 0);
}

void Socket::connect(const INetAddress& address, int32 port, int32 timeout)
{
    INetProt_Enum family = address.getFamily();

    if (family != m_family)
    {
        if (m_family == INET_PROT_IPV4)
        {
            throw INetException("Cannot connect IPv4 socket to IPv6 address");
        }
        else
        {
            throw INetException("Cannot connect IPv6 socket to IPv4 address");
        }
    }

    const unsigned char* addrData = address.getAddrData();
    int ret = 0;

    if (family == INET_PROT_IPV4)
    {
        sockaddr_in ipv4SockAddr;
        ::memset(&ipv4SockAddr, 0, sizeof(ipv4SockAddr));

        ipv4SockAddr.sin_family = AF_INET;
        ::memcpy(&ipv4SockAddr.sin_addr.S_un, addrData, 4);
        ipv4SockAddr.sin_port = htons(port);

        ret = ::connect(m_winSocket,
                        (const sockaddr*)&ipv4SockAddr,
                        sizeof(ipv4SockAddr));
    }
    else
    {
        sockaddr_in6 ipv6SockAddr;
        ::memset(&ipv6SockAddr, 0, sizeof(ipv6SockAddr));

        ipv6SockAddr.sin6_family = AF_INET6;
        ::memcpy(&ipv6SockAddr.sin6_addr.u, addrData, 16);
        ipv6SockAddr.sin6_port = htons(port);

        ret = ::connect(m_winSocket,
                        (const sockaddr*)&ipv6SockAddr,
                        sizeof(ipv6SockAddr));
    }

    if (ret)
    {
        throw INetException(WinUtil::getWSALastErrorMessage());
    }
}

void Socket::getConnectionAddress(INetAddress* address)
{
    (*address) = m_connAddress;
}

InputStream* Socket::getInputStream()
{
    if (m_inputStream != NULL)
        m_inputStream = new SocketInputStream(this);
    return m_inputStream;
}

OutputStream* Socket::getOutputStream()
{
    if (m_outputStream != NULL)
        m_outputStream = new SocketOutputStream(this);
    return m_outputStream;
}

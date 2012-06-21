// Socket.cpp

#include <ge/inet/Socket.h>

#include <ge/inet/INetException.h>
#include <gepriv/UnixUtil.h>

#include <cerrno>
#include <climits>
#include <cstring>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>


// Stream Implementations ---------------------------------------------------

/*
 * InputStream for a socket fd.
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
    SocketInputStream(const SocketInputStream&);
    SocketInputStream& operator=(const SocketInputStream&);

    Socket* m_socket;
};

SocketInputStream::SocketInputStream(Socket* socket) :
    m_socket(socket)
{
}

SocketInputStream::~SocketInputStream()
{
}

uint32 SocketInputStream::available()
{
    u_long ret;
    
    int ioctlRet = ::ioctl(m_socket->m_fd, FIONREAD, &ret);

    if (ioctlRet == -1)
    {
        throw INetException(String("ioctl() call failed with: ") +
                UnixUtil::getLastErrorMessage());
    }

    return (uint32)ret;
}

void SocketInputStream::close()
{
    m_socket->close();
}

int32 SocketInputStream::read()
{
    char buffer[1];
    return (int32)read(buffer, 1);
}

int64 SocketInputStream::read(void* buffer, uint32 len)
{
    ssize_t recvRet;

    // Prevent overflow to negative
    if (len > INT_MAX)
        len = INT_MAX;

    recvRet = UnixUtil::sys_recv(m_socket->m_fd, buffer, (size_t)len, 0);

    if (recvRet == -1)
    {
        throw INetException(String("recv() call failed with") +
                UnixUtil::getLastErrorMessage());
    }

    return recvRet;
}

/*
 * OutputStream for a socket fd.
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

    Socket* m_socket;
};

SocketOutputStream::SocketOutputStream(Socket* socket) :
    m_socket(socket)
{
}

SocketOutputStream::~SocketOutputStream()
{
}

void SocketOutputStream::close()
{
    m_socket->close();
}

int32 SocketOutputStream::write(int32 byte)
{
    char buffer[1] = {(char)byte};
    return (int32)write(buffer, 1);
}

int64 SocketOutputStream::write(const void* buffer, uint32 len)
{
    ssize_t sendRet;

    // Prevent overflow to negative
    if (len > INT_MAX)
        len = INT_MAX;

    sendRet = UnixUtil::sys_send(m_socket->m_fd, buffer, (size_t)len, 0);

    if (sendRet == -1)
    {
        throw INetException(String("send() call failed with: ") +
                UnixUtil::getLastErrorMessage());
    }

    return sendRet;
}

// Socket class implementation ----------------------------------------------

Socket::Socket() :
    m_fd(-1),
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
    m_fd = other.m_fd;
    other.m_fd = -1;
}

Socket& Socket::operator=(Socket&& other)
{
    if (this != &other)
    {
        m_fd = other.m_fd;
        other.m_fd = -1;
    }

    return *this;
}
#endif

void Socket::init(INetProt_Enum family)
{
    if (m_fd != -1)
    {
        throw INetException("Socket already initialized");
    }

    int prot = AF_INET;

    if (family == INET_PROT_IPV6)
    {
        prot = AF_INET6;
    }
    
    m_fd = ::socket(prot, SOCK_STREAM, IPPROTO_TCP);

    if (m_fd == -1)
    {
        throw INetException(UnixUtil::getLastErrorMessage());
    }

    m_family = family;
}

void Socket::close()
{
    char discardBuffer[512];

    // Start shutdown
    int shutRet = ::shutdown(m_fd, SHUT_WR);

    if (shutRet != 0)
    {
        // TODO: Log
    }

    // Receive until nothing to read or error
    ssize_t recvRet;

    do
    {
        recvRet = UnixUtil::sys_recv(m_fd, discardBuffer, sizeof(discardBuffer), 0);
    } while (recvRet > 0);

    if (recvRet == -1)
    {
        // TODO: Log
    }

    // Close the socket
    ::close(m_fd);
    m_fd = -1;

    delete m_inputStream;
    m_inputStream = NULL;
    delete m_outputStream;
    m_outputStream = NULL;
}

void Socket::listen()
{
    listen(SOMAXCONN);
}

void Socket::listen(int backlog)
{
    if (backlog <= 0)
    {
        backlog = SOMAXCONN;
    }

    int ret = ::listen(m_fd, backlog);

    if (ret == -1)
    {
        throw INetException(String("listen() call failed with: ") +
                UnixUtil::getLastErrorMessage());
    }
}

void Socket::accept(Socket* clientSocket)
{
    int32 sock;

    if (clientSocket->m_fd != -1)
    {
        throw INetException("Can't accept with uninitialized socket");
    }

    clientSocket->init(m_family);
    
    if (m_family == INET_PROT_IPV4)
    {
        sockaddr_in ipv4Address;
        ::memset(&ipv4Address, 0, sizeof(ipv4Address));

        int retAddrSize = sizeof(ipv4Address);
        sock = UnixUtil::sys_accept(m_fd, (sockaddr*)&ipv4Address, &retAddrSize);

        if (sock == -1)
        {
            throw INetException(UnixUtil::getLastErrorMessage());
        }

        ::memcpy(&clientSocket->m_connAddress.m_addr, &ipv4Address.sin_addr, 4);
        ::memset(&clientSocket->m_connAddress.m_addr+4, 0, 12);
    }
    else
    {
        sockaddr_in6 ipv6Address;
        ::memset(&ipv6Address, 0, sizeof(ipv6Address));

        int retAddrSize = sizeof(ipv6Address);
        sock = UnixUtil::sys_accept(m_fd, (sockaddr*)&ipv6Address, &retAddrSize);

        if (sock == -1)
        {
            throw INetException(UnixUtil::getLastErrorMessage());
        }

        ::memcpy(&clientSocket->m_connAddress.m_addr, &ipv6Address.sin6_addr, 16);
    }

    clientSocket->m_fd = sock;
    clientSocket->m_family = m_family;
    clientSocket->m_connAddress.m_family = m_family;
}

void Socket::bind(const INetAddress& address, int port)
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
        ::memcpy(&ipv4SockAddr.sin_addr, addrData, 4);
        ipv4SockAddr.sin_port = htons(port);

        ret = ::bind(m_fd,
                     (const sockaddr*)&ipv4SockAddr,
                     sizeof(ipv4SockAddr));
    }
    else
    {
        sockaddr_in6 ipv6SockAddr;
        ::memset(&ipv6SockAddr, 0, sizeof(ipv6SockAddr));

        ipv6SockAddr.sin6_family = AF_INET6;
        ::memcpy(&ipv6SockAddr.sin6_addr, addrData, 16);
        ipv6SockAddr.sin6_port = htons(port);

        ret = ::bind(m_fd,
                     (const sockaddr*)&ipv6SockAddr,
                     sizeof(ipv6SockAddr));
    }

    if (ret == -1)
    {
        throw INetException(UnixUtil::getLastErrorMessage());
    }
}

void Socket::connect(const INetAddress& address, int port)
{
    connect(address, port, 0);
}

void Socket::connect(const INetAddress& address, int port, int timeout)
{
    INetProt_Enum family;
    const unsigned char* addrData;

    sockaddr_in ipv4SockAddr;
    sockaddr_in6 ipv6SockAddr;

    const sockaddr* sockAddrPtr;
    socklen_t sockAddrLen;

    struct pollfd retry_pollfd;
    int connect_ret;
    int poll_ret;
    int getsockopt_ret;
    socklen_t connect_ret_len;


    family = address.getFamily();

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

    addrData = address.getAddrData();

    // Fill in the address information and prep the connect parameters
    if (family == INET_PROT_IPV4)
    {
        ::memset(&ipv4SockAddr, 0, sizeof(ipv4SockAddr));

        ipv4SockAddr.sin_family = AF_INET;
        ::memcpy(&ipv4SockAddr.sin_addr, addrData, 4);
        ipv4SockAddr.sin_port = htons(port);

        sockAddrPtr = (const sockaddr*)&ipv4SockAddr;
        sockAddrLen = sizeof(ipv4SockAddr);
    }
    else
    {
        ::memset(&ipv6SockAddr, 0, sizeof(ipv6SockAddr));

        ipv6SockAddr.sin6_family = AF_INET6;
        ::memcpy(&ipv6SockAddr.sin6_addr, addrData, 16);
        ipv6SockAddr.sin6_port = htons(port);

        sockAddrPtr = (const sockaddr*)&ipv6SockAddr;
        sockAddrLen = sizeof(ipv6SockAddr);
    }

    // Do the call to connect
    connect_ret = ::connect(m_fd, sockAddrPtr, sockAddrLen);

    // Linux seems to have forgiving retry behavior, but following the
    // spec here and, if interrupted, assuming the connection will
    // complete asynchronously, which requires us to use select, poll,
    // etc to wait for it to complete.
    if (connect_ret == -1 &&
        errno == EINTR)
    {
        retry_pollfd.fd = m_fd;
        retry_pollfd.events = POLLOUT;

        do
        {
            poll_ret = UnixUtil::sys_poll(&retry_pollfd, 1, -1);
        }
        while (poll_ret == -1 && errno == EINTR);

        if (poll_ret == -1)
        {
            throw INetException(String("connect() call interrupted and poll() "
                    "for asynchronous completion failed with: ") +
                    UnixUtil::getLastErrorMessage());
        }

        connect_ret_len = sizeof(connect_ret);
        getsockopt_ret = ::getsockopt(m_fd,
                                      SOL_SOCKET,
                                      SO_ERROR,
                                      &connect_ret,
                                      &connect_ret_len);

        if (getsockopt_ret == -1)
        {
            throw INetException(String("getsockopt() call failed with: ") +
                    UnixUtil::getLastErrorMessage());
        }

        // Fall through and test connect_ret
    }

    if (connect_ret)
    {
        throw INetException(String("connect() call failed with: ") +
                UnixUtil::getErrorMessage(connect_ret));
    }
}

void Socket::getConnectionAddress(INetAddress* address)
{
    (*address) = m_connAddress;
}

InputStream* Socket::getInputStream()
{
    if (m_inputStream == NULL)
        m_inputStream = new SocketInputStream(this);

    return m_inputStream;
}

OutputStream* Socket::getOutputStream()
{
    if (m_outputStream == NULL)
        m_outputStream = new SocketOutputStream(this);

    return m_outputStream;
}

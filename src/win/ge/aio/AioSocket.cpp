// AioSocket.cpp

#include "ge/aio/AioSocket.h"

#include "ge/aio/AioServer.h"
#include "ge/io/IOException.h"
#include "gepriv/WinUtil.h"

#include <ws2tcpip.h>

#define LISTEN_FLAG 0x1
#define BIND_FLAG   0x2

AioSocket::AioSocket() :
    _winSocket(INVALID_SOCKET),
    _owner(NULL),
    _flags(0)
{
}

AioSocket::~AioSocket()
{
    if (_owner != NULL)
    {
        // TODO: Log?
        _owner->dropSocket(this);
    }
}

#if defined(HAVE_RVALUE)
AioSocket::AioSocket(AioSocket&& other)
{

}

AioSocket& AioSocket::operator=(AioSocket&& other)
{
    if (&other != this)
    {

    }

    return *this;
}
#endif

void AioSocket::init(INetProt_Enum family)
{
    if (_winSocket != INVALID_SOCKET)
    {
        throw IOException("Socket already initialized");
    }

    int prot = AF_INET;

    if (family == INET_PROT_IPV6)
    {
        prot = AF_INET6;
    }
    
    _winSocket = ::WSASocket(prot, // IPv4 or IPv6 address family
                             SOCK_STREAM, // Stream connection
                             IPPROTO_TCP, // TCP protocol
                             0, // No protocol info
                             0, // No group
                             WSA_FLAG_OVERLAPPED); // Open for overlapped IO

    if (_winSocket == INVALID_SOCKET)
    {
        Error error = WinUtil::getError(::WSAGetLastError(),
                                        "WSASocket",
                                        "AioSocket::init");
        throw IOException(error);
    }

    // If Vista or later, change behavior to not queue IOCP entry if the
    // request completes immediately
#if (_WIN32_WINNT >= 0x0600)
    BOOL bret = ::SetFileCompletionNotificationModes((HANDLE)_winSocket,
            FILE_SKIP_COMPLETION_PORT_ON_SUCCESS);

    if (!bret)
    {
        Error error = WinUtil::getError(::WSAGetLastError(),
                "SetFileCompletionNotificationModes",
                "AioFile::open");
        throw IOException(error);
    }
#endif

    _family = family;
}

void AioSocket::hardClose()
{
    // Start shutdown
    int shutRet = ::shutdown(_winSocket, SD_SEND);

    if (shutRet != 0)
    {
        // TODO: Log
    }

    // Close the socket
    int closeRet = ::closesocket(_winSocket);

    if (closeRet != 0)
    {
        // TODO: Log
    }

    _winSocket = INVALID_SOCKET;
    _flags = 0;
}

void AioSocket::listen()
{
    listen(SOMAXCONN);
}

void AioSocket::listen(int backlog)
{
    if (_winSocket == INVALID_SOCKET)
    {
        throw IOException("Cannot listen on uninitialized socket");
    }

    if (backlog <= 0 ||
        backlog > SOMAXCONN)
    {
        backlog = SOMAXCONN;
    }

    int ret = ::listen(_winSocket, backlog);

    if (ret)
    {
        Error error = WinUtil::getError(::WSAGetLastError(),
                                        "listen",
                                        "AioSocket::listen");
        throw IOException(error);
    }

    _flags |= LISTEN_FLAG;
}

void AioSocket::bind(const INetAddress& address, int port)
{
    if (_winSocket == INVALID_SOCKET)
    {
        throw IOException("Cannot bind uninitialized socket");
    }

    INetProt_Enum family = address.getFamily();

    if (family != _family)
    {
        if (_family == INET_PROT_IPV4)
        {
            throw IOException("Cannot bind IPv4 socket to IPv6 address");
        }
        else
        {
            throw IOException("Cannot bind IPv6 socket to IPv4 address");
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

        ret = ::bind(_winSocket,
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

        ret = ::bind(_winSocket,
                     (const sockaddr*)&ipv6SockAddr,
                     sizeof(ipv6SockAddr));
    }

    if (ret)
    {
        Error error = WinUtil::getError(::WSAGetLastError(),
                                        "bind",
                                        "AioSocket::bind");
        throw IOException(error);
    }

    _flags |= BIND_FLAG;
}

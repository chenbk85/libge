// AioSocket.cpp

#include "ge/aio/AioSocket.h"

#include "ge/aio/AioServer.h"
#include "ge/io/IOException.h"
#include "gepriv/UnixUtil.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#define LISTEN_FLAG 0x1
#define BIND_FLAG   0x2

AioSocket::AioSocket() :
    _sockFd(-1),
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

/*
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
*/

void AioSocket::init(INetProt_Enum family)
{
    if (_sockFd != -1)
    {
        throw IOException("Socket already initialized");
    }

    int prot = AF_INET;

    if (family == INET_PROT_IPV6)
    {
        prot = AF_INET6;
    }

    // TODO: Linux should mask on SOCK_NONBLOCK, SOCK_CLOEXEC

    _sockFd = ::socket(prot, // Protocol family
                       SOCK_STREAM, // Type of connection
                       0); // Protocol (0 for normal IP)

    if (_sockFd == -1)
    {
        Error error = UnixUtil::getError(errno,
                                         "socket",
                                         "AioSocket::init");
        throw IOException(error);
    }

    // TODO: Avoid if recent Linux
    int res = ::fcntl(_sockFd, F_SETFD, O_NONBLOCK | FD_CLOEXEC);

    if (res != 0)
    {
        Error error = UnixUtil::getError(errno,
                                         "fcntl",
                                         "AioSocket::init");
        throw IOException(error);
    }

    _family = family;
}

void AioSocket::hardClose()
{
    // Start shutdown
    int shutRet = ::shutdown(_sockFd, SHUT_WR);

    if (shutRet != 0)
    {
        // TODO: Log
    }

    // Close the socket
    int closeRet = ::close(_sockFd);

    if (closeRet != 0)
    {
        // TODO: Log
    }

    _sockFd = -1;
    _flags = 0;
}

void AioSocket::listen()
{
    // TODO: If linux pass INT_MAX as it gets truncated
    listen(SOMAXCONN);
}

void AioSocket::listen(int backlog)
{
    if (_sockFd == -1)
    {
        throw IOException("Cannot listen on uninitialized socket");
    }

    // TODO: If not Linux
    if (backlog <= 0 ||
        backlog > SOMAXCONN)
    {
        backlog = SOMAXCONN;
    }

    int ret = ::listen(_sockFd, backlog);

    if (ret)
    {
        Error error = UnixUtil::getError(errno,
                                         "listen",
                                         "AioSocket::listen");
        throw IOException(error);
    }

    _flags |= LISTEN_FLAG;
}

void AioSocket::bind(const INetAddress& address, int port)
{
    if (_sockFd == -1)
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
        ::memcpy(&ipv4SockAddr.sin_addr.s_addr, addrData, 4);
        ipv4SockAddr.sin_port = htons(port);

        ret = ::bind(_sockFd,
                     (const sockaddr*)&ipv4SockAddr,
                     sizeof(ipv4SockAddr));
    }
    else
    {
        sockaddr_in6 ipv6SockAddr;
        ::memset(&ipv6SockAddr, 0, sizeof(ipv6SockAddr));

        ipv6SockAddr.sin6_family = AF_INET6;
        ::memcpy(&ipv6SockAddr.sin6_addr.s6_addr, addrData, 16);
        ipv6SockAddr.sin6_port = htons(port);

        ret = ::bind(_sockFd,
                     (const sockaddr*)&ipv6SockAddr,
                     sizeof(ipv6SockAddr));
    }

    if (ret)
    {
        Error error = UnixUtil::getError(errno,
                                         "bind",
                                         "AioSocket::bind");
        throw IOException(error);
    }

    _flags |= BIND_FLAG;
}

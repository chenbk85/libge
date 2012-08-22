// SocketServicePoll.cpp

#include "gepriv/aio/SocketServicePoll.h"

#include "ge/io/IOException.h"
#include "ge/util/Locker.h"
#include "gepriv/UnixUtil.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>

#define FLAG_ACCEPT 0x1
#define FLAG_CONNECT 0x2
#define FLAG_READ 0x4
#define FLAG_WRITE 0x8
#define FLAG_SENDFILE 0x10


SocketServicePoll::SocketServicePoll(AioServer* aioServer) :
    _aioServer(aioServer)
{
    int pipeRes = ::pipe(_wakeupPipe);

    if (pipeRes != 0)
    {
        Error error = UnixUtil::getError(errno,
                                         "pipe",
                                         "AioServer::AioServer");
        throw IOException(error);
    }

    for (int i = 0; i < 2; i++)
    {
        int fcntlRes = ::fcntl(_wakeupPipe[i], F_SETFD, O_NONBLOCK | FD_CLOEXEC);

        if (fcntlRes != 0)
        {
            Error error = UnixUtil::getError(errno,
                                             "fcntl",
                                             "AioServer::AioServer");
            throw IOException(error);
        }
    }
}

SocketServicePoll::~SocketServicePoll()
{
    ::close(_wakeupPipe[0]);
    ::close(_wakeupPipe[1]);
}

void SocketServicePoll::process()
{

}

void SocketServicePoll::shutdown()
{

}

void SocketServicePoll::submitAccept(AioSocket* listenSocket,
                                     AioSocket* acceptSocket,
                                     AioServer::acceptCallback callback,
                                     void* userData)
{
    Locker<Mutex> locker(_lock);

    HashMap<int, SockData>::Iterator iter = _dataMap.get(listenSocket->_sockFd);

    if (iter.isValid())
    {
        HashMap<int, SockData>::Entry entry = iter.value();
        SockData& sockData = entry.getValue();

        // TODO: Should check socket state?
        if (sockData.operMask != 0)
        {
            throw IOException("Cannot accept on socket performing another operation");
        }

        sockData.operMask = POLLIN;
        sockData.readOper.operType = FLAG_ACCEPT;
        sockData.readOper.callback = (void*)callback;
        sockData.readOper.aioSocket = listenSocket;
        sockData.readOper.acceptSocket = acceptSocket;
        sockData.readOper.userData = userData;
    }
    else
    {
        SockData newData;

        newData.operMask = POLLIN;
        newData.readOper.operType = FLAG_ACCEPT;
        newData.readOper.callback = (void*)callback;
        newData.readOper.aioSocket = listenSocket;
        newData.readOper.acceptSocket = acceptSocket;
        newData.readOper.userData = userData;

        _dataMap.put(listenSocket->_sockFd, newData);
    }

    locker.unlock();
    wakeup();
}

void SocketServicePoll::submitConnect(AioSocket* aioSocket,
                                      AioServer::connectCallback callback,
                                      void* userData,
                                      const INetAddress& address,
                                      int32 port)
{
    submitConnect(aioSocket,
                  callback,
                  userData,
                  address,
                  port,
                  0);
}

void SocketServicePoll::submitConnect(AioSocket* aioSocket,
                                      AioServer::connectCallback callback,
                                      void* userData,
                                      const INetAddress& address,
                                      int32 port,
                                      int32 timeout)
{
    Locker<Mutex> locker(_lock);

    HashMap<int, SockData>::Iterator iter = _dataMap.get(aioSocket->_sockFd);

    if (iter.isValid())
    {
        HashMap<int, SockData>::Entry entry = iter.value();
        SockData& sockData = entry.getValue();

        // TODO: Should check socket state?
        if (sockData.operMask != 0)
        {
            throw IOException("Cannot connect on socket performing another operation");
        }

        sockData.operMask = POLLOUT;
        sockData.writeOper.operType = FLAG_CONNECT;
        sockData.writeOper.callback = (void*)callback;
        sockData.writeOper.aioSocket = aioSocket;
        sockData.writeOper.userData = userData;
    }
    else
    {
        SockData newData;

        newData.operMask = POLLOUT;
        newData.writeOper.operType = FLAG_CONNECT;
        newData.writeOper.callback = (void*)callback;
        newData.writeOper.aioSocket = aioSocket;
        newData.writeOper.userData = userData;

        _dataMap.put(aioSocket->_sockFd, newData);
    }

    locker.unlock();
    wakeup();
}

void SocketServicePoll::socketRead(AioSocket* aioSocket,
                                   AioServer::socketCallback callback,
                                   void* userData,
                                   char* buffer,
                                   uint32 bufferLen)
{
    Locker<Mutex> locker(_lock);

    HashMap<int, SockData>::Iterator iter = _dataMap.get(aioSocket->_sockFd);

    if (iter.isValid())
    {
        HashMap<int, SockData>::Entry entry = iter.value();
        SockData& sockData = entry.getValue();

        // TODO: Should check socket state?

        sockData.operMask |= POLLIN;
        sockData.readOper.operType = FLAG_READ;
        sockData.readOper.callback = (void*)callback;
        sockData.readOper.aioSocket = aioSocket;
        sockData.readOper.userData = userData;
        sockData.readOper.buffer = buffer;
        sockData.readOper.bufferPos = 0;
        sockData.readOper.bufferLen = bufferLen;
    }
    else
    {
        SockData newData;

        newData.operMask = POLLIN;
        newData.readOper.operType = FLAG_READ;
        newData.readOper.callback = (void*)callback;
        newData.readOper.aioSocket = aioSocket;
        newData.readOper.userData = userData;
        newData.readOper.buffer = buffer;
        newData.readOper.bufferPos = 0;
        newData.readOper.bufferLen = bufferLen;

        _dataMap.put(aioSocket->_sockFd, newData);
    }

    locker.unlock();
    wakeup();
}

void SocketServicePoll::socketWrite(AioSocket* aioSocket,
                                    AioServer::socketCallback callback,
                                    void* userData,
                                    const char* buffer,
                                    uint32 bufferLen)
{
    Locker<Mutex> locker(_lock);

    HashMap<int, SockData>::Iterator iter = _dataMap.get(aioSocket->_sockFd);

    if (iter.isValid())
    {
        HashMap<int, SockData>::Entry entry = iter.value();
        SockData& sockData = entry.getValue();

        // TODO: Should check socket state?

        sockData.operMask |= POLLOUT;
        sockData.writeOper.operType = FLAG_WRITE;
        sockData.writeOper.callback = (void*)callback;
        sockData.writeOper.aioSocket = aioSocket;
        sockData.writeOper.userData = userData;
        sockData.writeOper.buffer = (char*)buffer;
        sockData.writeOper.bufferPos = 0;
        sockData.writeOper.bufferLen = bufferLen;
    }
    else
    {
        SockData newData;

        newData.operMask = POLLOUT;
        newData.writeOper.operType = FLAG_WRITE;
        newData.writeOper.callback = (void*)callback;
        newData.writeOper.aioSocket = aioSocket;
        newData.writeOper.userData = userData;
        newData.writeOper.buffer = (char*)buffer;
        newData.writeOper.bufferPos = 0;
        newData.writeOper.bufferLen = bufferLen;

        _dataMap.put(aioSocket->_sockFd, newData);
    }

    locker.unlock();
    wakeup();
}

void SocketServicePoll::socketSendFile(AioSocket* aioSocket,
                                       AioServer::socketCallback callback,
                                       void* userData,
                                       AioFile* aioFile,
                                       uint64 pos,
                                       uint32 writeLen)
{

}

void SocketServicePoll::emptyWakePipe()
{
    char buffer[255];
    int res;

    do
    {
        res = ::read(_wakeupPipe[0], buffer, sizeof(buffer));
    } while (res == -1 && errno == EINTR);
}

void SocketServicePoll::wakeup()
{
    char data[1] = {'1'};
    int res;

    do
    {
        res = ::write(_wakeupPipe[1], data, 1);
    } while (res == -1 && errno == EINTR);
}

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

    HashMap<int, SockData>::Iterator iter = _dataMap.get(listenSocket->_fd);

    if (iter.isValid())
    {
        HashMap<int, SockData>::Entry entry = iter.value();

        // TODO: Should check socket state?
        if (entry._value.operMask != 0)
        {
            throw IOException("Cannot accept on socket performing another operation");
        }

        entry._value.readOper = FLAG_ACCEPT;
        entry._value.operMask = POLLIN;
        entry._value.readOper.callback = callback;
        entry._value.readOper.aioSocket = listenSocket;
        entry._value.readOper.acceptSocket = acceptSocket;
        entry._value.readOper.userData = userData;
    }
    else
    {
        SockData newData = new SockData();
        newData.readOper = FLAG_ACCEPT;
        newData.operMask = POLLIN;
        newData.readOper.aioSocket = listenSocket;
        newData.readOper.acceptSocket = acceptSocket;
        newData.readOper.callback = callback;
        newData.readOper.userData = userData;

        try
        {
            _dataMap.put(listenSocket->_fd, newData);
        }
        catch (...)
        {
            delete newData;
            throw;
        }
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
    /*
    Locker<Mutex> locker(_lock);

    HashMap<int, SockData>::Iterator iter = _dataMap.get(listenSocket->_fd);

    if (iter.isValid())
    {
        HashMap<int, SockData>::Entry entry = iter.value();

        // TODO: Should check socket state?
        if (entry._value.operMap != 0)
        {
            throw IOException("Cannot connect on socket performing another operation");
        }

        entry._value.operMap = FLAG_CONNECT;
        entry._value.writeCallback = callback;
    }
    else
    {
        SockData* newData = new SockData();
        newData->aioSocket = listenSocket;
        newData->writeCallback = callback;
        newData->writeUserData = userData;

        try
        {
            _dataMap.put(listenSocket->_fd, newData);
        }
        catch (...)
        {
            delete newData;
            throw;
        }
    }

    locker.unlock();
    wakeup();
    */
}

void SocketServicePoll::socketRead(AioSocket* aioSocket,
                                   AioServer::socketCallback callback,
                                   void* userData,
                                   char* buffer,
                                   uint32 bufferLen)
{

}

void SocketServicePoll::socketWrite(AioSocket* aioSocket,
                                    AioServer::socketCallback callback,
                                    void* userData,
                                    const char* buffer,
                                    uint32 bufferLen)
{

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

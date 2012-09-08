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

#define SEND_FILE_BUF_LEN 2048


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
        sockData.readOper = FLAG_ACCEPT;
        sockData.readCallback = (void*)callback;
        sockData.acceptSocket = acceptSocket;
        sockData.readUserData = userData;
    }
    else
    {
        SockData newData;

        newData.aioSocket = listenSocket;
        newData.operMask = POLLIN;
        newData.readOper = FLAG_ACCEPT;
        newData.readCallback = (void*)callback;
        newData.acceptSocket = acceptSocket;
        newData.readUserData = userData;

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
        sockData.writeOper = FLAG_CONNECT;
        sockData.writeCallback = (void*)callback;
        sockData.writeUserData = userData;
    }
    else
    {
        SockData newData;

        newData.aioSocket = aioSocket;
        newData.operMask = POLLOUT;
        newData.writeOper = FLAG_CONNECT;
        newData.writeCallback = (void*)callback;
        newData.writeUserData = userData;

        _dataMap.put(aioSocket->_sockFd, newData);
    }

    // Call connect

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
        sockData.readOper = FLAG_READ;
        sockData.readCallback = (void*)callback;
        sockData.readUserData = userData;
        sockData.readBuffer = buffer;
        sockData.readBufferPos = 0;
        sockData.readBufferLen = bufferLen;
    }
    else
    {
        SockData newData;

        newData.aioSocket = aioSocket;
        newData.operMask = POLLIN;
        newData.readOper = FLAG_READ;
        newData.readCallback = (void*)callback;
        newData.readUserData = userData;
        newData.readBuffer = buffer;
        newData.readBufferPos = 0;
        newData.readBufferLen = bufferLen;

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
        sockData.writeOper = FLAG_WRITE;
        sockData.writeCallback = (void*)callback;
        sockData.writeUserData = userData;
        sockData.writeBuffer = (char*)buffer;
        sockData.writeBufferPos = 0;
        sockData.writeBufferLen = bufferLen;
    }
    else
    {
        SockData newData;

        newData.aioSocket = aioSocket;
        newData.operMask = POLLOUT;
        newData.writeOper = FLAG_WRITE;
        newData.writeCallback = (void*)callback;
        newData.writeUserData = userData;
        newData.writeBuffer = (char*)buffer;
        newData.writeBufferPos = 0;
        newData.writeBufferLen = bufferLen;

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
    Locker<Mutex> locker(_lock);

    // TODO: Check if file is open

    HashMap<int, SockData>::Iterator iter = _dataMap.get(aioSocket->_sockFd);

    if (iter.isValid())
    {
        HashMap<int, SockData>::Entry entry = iter.value();
        SockData& sockData = entry.getValue();

        // TODO: Should check socket state?

        sockData.operMask |= POLLOUT;
        sockData.writeOper = FLAG_SENDFILE;
        sockData.writeCallback = (void*)callback;
        sockData.writeUserData = userData;

        if (sockData.sendFileBuf == NULL)
            sockData.sendFileBuf = new char[SEND_FILE_BUF_LEN];

        sockData.sendFileBufFilled = 0;
        sockData.sendFileBufIndex = 0;
        sockData.sendFileOffset = pos;
        sockData.sendFileEnd = pos + writeLen;
    }
    else
    {
        SockData newData;

        newData.aioSocket = aioSocket;
        newData.operMask = POLLOUT;
        newData.writeOper = FLAG_SENDFILE;
        newData.writeCallback = (void*)callback;
        newData.writeUserData = userData;

        newData.sendFileBuf = new char[SEND_FILE_BUF_LEN];
        newData.sendFileBufFilled = 0;
        newData.sendFileBufIndex = 0;
        newData.sendFileOffset = pos;
        newData.sendFileEnd = pos + writeLen;

        _dataMap.put(aioSocket->_sockFd, newData);
    }

    locker.unlock();
    wakeup();
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

// Inner Classes ------------------------------------------------------------

SocketServicePoll::SockData::SockData() :
    readyNext(NULL),
    aioSocket(NULL),
    operMask(0),
    readOper(0),
    acceptSocket(NULL),
    readCallback(NULL),
    readUserData(NULL),
    readBuffer(NULL),
    readBufferPos(0),
    readBufferLen(0),
    writeOper(0),
    writeCallback(NULL),
    writeUserData(NULL),
    writeBuffer(NULL),
    writeBufferPos(0),
    writeBufferLen(0),
    sendFileFd(-1),
    sendFileBuf(NULL),
    sendFileBufFilled(0),
    sendFileBufIndex(0),
    sendFileOffset(0),
    sendFileEnd(0)
{

}

SocketServicePoll::SockData::~SockData()
{
    delete[] sendFileBuf;
}

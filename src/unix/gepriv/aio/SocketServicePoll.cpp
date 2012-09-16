// SocketServicePoll.cpp

#include "gepriv/aio/SocketServicePoll.h"

#include "ge/io/IOException.h"
#include "ge/thread/CurrentThread.h"
#include "ge/util/Locker.h"
#include "gepriv/UnixUtil.h"

#include <climits>
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define FLAG_ACCEPT 0x1
#define FLAG_CONNECT 0x2
#define FLAG_READ 0x4
#define FLAG_WRITE 0x8
#define FLAG_SENDFILE 0x10

#define SEND_FILE_BUF_LEN 2048


SocketService::SocketService() :
    _isShutdown(false),
    _pollWorker(this)
{
}

SocketService::~SocketService()
{
    ::close(_wakeupPipe[0]);
    ::close(_wakeupPipe[1]);
}

void SocketService::startServing(uint32 desiredThreads)
{
    Locker<Condition> locker(_cond);

    if (_isShutdown)
        throw IOException("Cannot restart shutdown SocketService");

    // Create the wakeup pipe
    int pipeRes = ::pipe(_wakeupPipe);

    if (pipeRes != 0)
    {
        Error error = UnixUtil::getError(errno,
                                         "pipe",
                                         "SocketService::AioServer");
        throw IOException(error);
    }

    for (int i = 0; i < 2; i++)
    {
        int fcntlRes = ::fcntl(_wakeupPipe[i], F_SETFD, O_NONBLOCK | FD_CLOEXEC);

        if (fcntlRes != 0)
        {
            Error error = UnixUtil::getError(errno,
                                             "fcntl",
                                             "SocketService::AioServer");
            throw IOException(error);
        }
    }

    // Create worker threads
    // If this throws we're depending on the destructor for cleanup
    for (uint32 i = 0; i < desiredThreads; i++)
    {
        AioWorker* worker = new AioWorker(this);
        _threads.addBack(worker);

        worker->start();
    }
}

void SocketService::shutdown()
{
    // Signal shutdown
    Locker<Condition> locker(_cond);

    _isShutdown = true;

    if (_threads.isEmpty())
        return;

    _cond.signalAll();

    locker.unlock();

    // Join and delete threads
    size_t threadCount = _threads.size();
    for (size_t i = 0; i < threadCount; i++)
    {
        AioWorker* worker = _threads.get(i);
        worker->join();
        delete worker;
    }

    _threads.clear();
}

void SocketService::socketAccept(AioSocket* listenSocket,
                                     AioSocket* acceptSocket,
                                     SocketService::acceptCallback callback,
                                     void* userData)
{
    if (listenSocket->_sockFd != -1)
    {
        throw IOException("Can't accept with uninitialized socket");
    }

    // TODO: check if acceptSocket is initialized wrong

    acceptSocket->init(listenSocket->_family);

    Locker<Condition> locker(_cond);

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

void SocketService::socketConnect(AioSocket* aioSocket,
                                      SocketService::connectCallback callback,
                                      void* userData,
                                      const INetAddress& address,
                                      int32 port)
{
    Locker<Condition> locker(_cond);

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
        sockData.connectAddress = address;
        sockData.connectPort = port;
    }
    else
    {
        SockData newData;

        newData.aioSocket = aioSocket;
        newData.operMask = POLLOUT;
        newData.writeOper = FLAG_CONNECT;
        newData.writeCallback = (void*)callback;
        newData.writeUserData = userData;
        newData.connectAddress = address;
        newData.connectPort = port;

        _dataMap.put(aioSocket->_sockFd, newData);
    }

    locker.unlock();
    wakeup();
}

void SocketService::socketRead(AioSocket* aioSocket,
                               SocketService::socketCallback callback,
                               void* userData,
                               char* buffer,
                               uint32 bufferLen)
{
    Locker<Condition> locker(_cond);

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

void SocketService::socketWrite(AioSocket* aioSocket,
                                SocketService::socketCallback callback,
                                void* userData,
                                const char* buffer,
                                uint32 bufferLen)
{
    Locker<Condition> locker(_cond);

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

void SocketService::socketSendFile(AioSocket* aioSocket,
                                   SocketService::socketCallback callback,
                                   void* userData,
                                   AioFile* aioFile,
                                   uint64 pos,
                                   uint32 writeLen)
{
    Locker<Condition> locker(_cond);

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

void SocketService::emptyWakePipe()
{
    char buffer[255];
    int res;

    do
    {
        res = ::read(_wakeupPipe[0], buffer, sizeof(buffer));
    } while (res == -1 && errno == EINTR);
}

void SocketService::wakeup()
{
    char data[1] = {'1'};
    int res;

    do
    {
        res = ::write(_wakeupPipe[1], data, 1);
    } while (res == -1 && errno == EINTR);
}

bool SocketService::handleAcceptReady(SockData* sockData, Error& error)
{
    INetProt_Enum family;
    sockaddr_in ipv4Address;
    sockaddr_in6 ipv6Address;
    sockaddr* addrPtr;
    socklen_t addrSize;
    socklen_t* retAddrSize;
    int ret;
    int err;

    family = sockData->aioSocket->_family;
    retAddrSize = &addrSize;

    // Set up the address
    if (family == INET_PROT_IPV4)
    {
        ::memset(&ipv4Address, 0, sizeof(ipv4Address));
        addrPtr = (sockaddr*)&ipv4Address;
        addrSize = sizeof(ipv4Address);
    }
    else
    {
        ::memset(&ipv6Address, 0, sizeof(ipv6Address));
        addrPtr = (sockaddr*)&ipv6Address;
        addrSize = sizeof(ipv6Address);
    }

    // Accept
    do
    {
        ret = ::accept(sockData->aioSocket->_sockFd, addrPtr, retAddrSize);
    } while (ret == -1 && errno == EINTR);

    if (ret != -1)
    {
        // Accept succeeded
        sockData->acceptSocket->_sockFd = ret;
        return true;
    }
    else
    {
        // Handle error
        err = errno;

        // If the error is that it would block, just keep going
        if (err != EAGAIN || err != EWOULDBLOCK)
        {
            error = UnixUtil::getError(errno,
                "SocketService::accept",
                "accept");
            return true;
        }
    }

    return false;
}

bool SocketService::handleConnectReady(SockData* sockData, Error& error)
{
    sockaddr_in ipv4SockAddr;
    sockaddr_in6 ipv6SockAddr;

    INetProt_Enum family;
    const sockaddr* sockAddrPtr;
    socklen_t sockAddrLen;

    const unsigned char* addrData;
    int port;
    int res;
    int err;

    family = sockData->aioSocket->_family;
    addrData = sockData->connectAddress.getAddrData();
    port = sockData->connectPort;

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
    do
    {
        res = ::connect(sockData->aioSocket->_sockFd, sockAddrPtr, sockAddrLen);
    }
    while (res == -1 && errno == EINTR);

    if (res == 0)
    {
        // Success
        return true;
    }
    else
    {
        err = errno;

        if (err != EINPROGRESS)
        {
            error = UnixUtil::getError(err,
                "SocketService::connect",
                "connect");
            return true;
        }
    }
    return false;
}

bool SocketService::handleReadReady(SockData* sockData, Error& error)
{
    ssize_t res;
    int err;
    size_t recvLen;

    recvLen = sockData->readBufferLen;

    // Prevent overflow to negative
    if (recvLen > INT_MAX)
        recvLen = INT_MAX;

    do
    {
        res = ::recv(sockData->aioSocket->_sockFd,
                     sockData->readBuffer,
                     recvLen,
                     0);
    }
    while (res == -1 && errno == EINTR);

    if (res != -1)
    {
        sockData->writeBufferPos = res;
        return true;
    }
    else
    {
        err = errno;

        if (err != EAGAIN &&
            err != EWOULDBLOCK)
        {
            error = UnixUtil::getError(err,
                "SocketService::read",
                "recv");
            return true;
        }
    }

    return false;
}

bool SocketService::handleWriteReady(SockData* sockData, Error& error)
{
    return false;
}

bool SocketService::handleSendfileReady(SockData* sockData, Error& error)
{
    return false;
}

void SocketService::dropSocket(AioSocket* aioSocket)
{

}

bool SocketService::process()
{
    while (true)
    {
        Locker<Condition> locker(_cond);

        while (!_isShutdown &&
               _readyQueueHead == NULL)
        {
            _cond.wait();
        }

        if (_isShutdown)
            return false;

        // Pop an entry from the queue
        QueueEntry* queueEntry = _readyQueueHead;
        _readyQueueHead = queueEntry->next;

        if (_readyQueueHead == NULL)
        {
            _readyQueueTail = NULL;
        }
        else
        {
            _readyQueueHead->prev = NULL;
        }

        locker.unlock();

        // Take action depending on the data
        SockData* sockData = queueEntry->data;
        bool isRead = queueEntry->isRead;

        Error error;
        bool operComplete = false;

        if (isRead)
        {
            if (sockData->readOper == FLAG_ACCEPT)
            {
                operComplete = handleAcceptReady(sockData, error);

                if (operComplete)
                {
                    SocketService::acceptCallback callback = (SocketService::acceptCallback)sockData->readCallback;
                    callback(sockData->aioSocket,
                             sockData->acceptSocket,
                             sockData->readUserData,
                             error);
                }
            }
            else if (sockData->readOper == FLAG_READ)
            {
                operComplete = handleReadReady(sockData, error);

                if (operComplete)
                {
                    SocketService::socketCallback callback = (SocketService::socketCallback)sockData->readCallback;
                    callback(sockData->aioSocket,
                             sockData->readUserData,
                             sockData->readBufferPos,
                             error);
                }
            }
        }
        else
        {

        }
    }

    return true;
}

bool SocketService::poll()
{
    return false;
}

// Inner Classes ------------------------------------------------------------

SocketService::AioWorker::AioWorker(SocketService* socketService) :
    _socketService(socketService)
{
}

void SocketService::AioWorker::run()
{
    CurrentThread::setName("SocketService Worker");

    bool keepGoing = true;

    while (keepGoing)
    {
        keepGoing = _socketService->process();
    }
}

SocketService::PollWorker::PollWorker(SocketService* socketService) :
    _socketService(socketService)
{

}

void SocketService::PollWorker::run()
{
    CurrentThread::setName("FileService Poll Worker");

    bool keepGoing = true;

    while (keepGoing)
    {
        keepGoing = _socketService->poll();
    }
}

SocketService::SockData::SockData() :
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
    connectPort(0),
    sendFileFd(-1),
    sendFileBuf(NULL),
    sendFileBufFilled(0),
    sendFileBufIndex(0),
    sendFileOffset(0),
    sendFileEnd(0)
{
    readQueueEntry.isRead = true;
    readQueueEntry.data = this;
    readQueueEntry.prev = NULL;
    readQueueEntry.next = NULL;
    writeQueueEntry.isRead = false;
    writeQueueEntry.data = this;
    writeQueueEntry.prev = NULL;
    writeQueueEntry.next = NULL;
}

SocketService::SockData::~SockData()
{
    delete[] sendFileBuf;
}

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

    SockData newData;
    

    Locker<Condition> locker(_cond);

    // TODO: Should check socket state?

    HashMap<int, SockData>::Iterator iter = _dataMap.get(listenSocket->_sockFd);

    if (iter.isValid())
    {
        throw IOException("Cannot accept on socket performing another operation");
    }

    // Create a SockData object
    SockData sockData;
    sockData.aioSocket = listenSocket;
    sockData.readOper = FLAG_ACCEPT;
    sockData.readCallback = (void*)callback;
    sockData.acceptSocket = acceptSocket;
    sockData.readUserData = userData;

    // Try to accept
    doAccept(&sockData);

    // Add to the data map and wake the poller if didn't immediately complete 
    if (!sockData.readComplete)
    {
        _dataMap.put(listenSocket->_sockFd, newData);
        wakeup();
    }
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
        throw IOException("Cannot connect on socket performing another operation");
    }

    SockData sockData;
    sockData.aioSocket = aioSocket;
    sockData.writeOper = FLAG_CONNECT;
    sockData.writeCallback = (void*)callback;
    sockData.writeUserData = userData;
    sockData.connectAddress = address;
    sockData.connectPort = port;

    // Try to connect
    doConnect(&sockData);

    // Add to the data map and wake the poller if didn't immediately complete 
    if (!sockData.writeComplete)
    {
        _dataMap.put(aioSocket->_sockFd, sockData);
        wakeup();
    }
}

void SocketService::socketRead(AioSocket* aioSocket,
                               SocketService::socketCallback callback,
                               void* userData,
                               char* buffer,
                               uint32 bufferLen)
{
    Locker<Condition> locker(_cond);

    SockData newData;
    SockData& sockData = newData;

    // TODO: Should check socket state?

    HashMap<int, SockData>::Iterator iter = _dataMap.get(aioSocket->_sockFd);

    if (iter.isValid())
    {
        HashMap<int, SockData>::Entry entry = iter.value();
        sockData = entry.getValue();

        if (sockData.readOper != 0)
        {
            throw IOException("Cannot read from socket with read operation already in progress");
        }
    }

    sockData.aioSocket = aioSocket;
    sockData.readOper = FLAG_READ;
    sockData.readCallback = (void*)callback;
    sockData.readUserData = userData;
    sockData.readBuffer = buffer;
    sockData.readBufferPos = 0;
    sockData.readBufferLen = bufferLen;

    // Try to recv
    doRecv(&sockData);

    // Add to the data map and wake the poller if didn't immediately complete 
    if (!sockData.readComplete &&
        !iter.isValid())
    {
        _dataMap.put(aioSocket->_sockFd, newData);
        wakeup();
    }
}

void SocketService::socketWrite(AioSocket* aioSocket,
                                SocketService::socketCallback callback,
                                void* userData,
                                const char* buffer,
                                uint32 bufferLen)
{
    Locker<Condition> locker(_cond);

    SockData newData;
    SockData& sockData = newData;

    // TODO: Should check socket state?

    HashMap<int, SockData>::Iterator iter = _dataMap.get(aioSocket->_sockFd);

    if (iter.isValid())
    {
        HashMap<int, SockData>::Entry entry = iter.value();
        SockData& sockData = entry.getValue();

        if (sockData.writeOper != 0)
        {
            throw IOException("Cannot write to socket with write operation already in progress");
        }
    }
    
    sockData.aioSocket = aioSocket;
    sockData.writeOper = FLAG_WRITE;
    sockData.writeCallback = (void*)callback;
    sockData.writeUserData = userData;
    sockData.writeBuffer = (char*)buffer;
    sockData.writeBufferPos = 0;
    sockData.writeBufferLen = bufferLen;

    // Try to recv
    doSend(&sockData);

    // Add to the data map and wake the poller if didn't immediately complete 
    if (!sockData.writeComplete &&
        !iter.isValid())
    {
        _dataMap.put(aioSocket->_sockFd, newData);
        wakeup();
    }
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
        if (sockData.writeOper != 0)
        {
            throw IOException("Cannot write to socket with write operation already in progress");
        }

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

void SocketService::doAccept(SockData* sockData)
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
        sockData->readComplete = true;
    }
    else
    {
        // Handle error
        err = errno;

        // If the error is that it would block, just keep going
        if (err != EAGAIN || err != EWOULDBLOCK)
        {
            sockData->readError = UnixUtil::getError(errno,
                "SocketService::accept",
                "accept");
            sockData->readComplete = true;
        }
    }
}

void SocketService::doConnect(SockData* sockData)
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
        sockData->writeComplete = true;
    }
    else
    {
        err = errno;

        if (err != EINPROGRESS)
        {
            sockData->writeError = UnixUtil::getError(err,
                "SocketService::connect",
                "connect");
            sockData->writeComplete = true;
        }
    }
}

void SocketService::doRecv(SockData* sockData)
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
        sockData->readBufferPos = res;
        sockData->readComplete = true;
    }
    else
    {
        err = errno;

        if (err != EAGAIN &&
            err != EWOULDBLOCK)
        {
            sockData->readError = UnixUtil::getError(err,
                "SocketService::submitRecv",
                "recv");
            sockData->readComplete = true;
        }
    }
}

void SocketService::doSend(SockData* sockData)
{
    ssize_t res;
    int err;
    size_t sendLen;

    sendLen = sockData->writeBufferLen - sockData->writeBufferPos;

    // Prevent overflow to negative
    if (sendLen > INT_MAX)
        sendLen = INT_MAX;

    int flags = 0;

#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#endif

    do
    {
        res = ::send(sockData->aioSocket->_sockFd,
                     sockData->writeBuffer + sockData->writeBufferPos,
                     sendLen,
                     flags);
    }
    while (res == -1 && errno == EINTR);

    if (res != -1)
    {
        sockData->writeBufferPos += res;
        sockData->writeComplete = true;
    }
    else
    {
        err = errno;

        if (err != EAGAIN &&
            err != EWOULDBLOCK)
        {
            sockData->writeError = UnixUtil::getError(err,
                "SocketService::submitSend",
                "send");
            sockData->writeComplete = true;
        }
    }
}

void SocketService::doSendfile(SockData* sockData)
{

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
                doAccept(sockData);

                if (sockData->readComplete)
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
                doRecv(sockData);

                if (sockData->readComplete)
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
            if (sockData->writeOper == FLAG_CONNECT)
            {
                doAccept(sockData);

                if (sockData->writeComplete)
                {
                    SocketService::acceptCallback callback = (SocketService::acceptCallback)sockData->readCallback;
                    callback(sockData->aioSocket,
                             sockData->acceptSocket,
                             sockData->writeUserData,
                             error);
                }
            }
            else if (sockData->writeOper == FLAG_WRITE)
            {
                doSend(sockData);

                if (sockData->writeComplete)
                {
                    SocketService::connectCallback callback = (SocketService::connectCallback)sockData->readCallback;
                    callback(sockData->aioSocket,
                             sockData->writeUserData,
                             error);
                }
            }
            else if (sockData->writeOper == FLAG_SENDFILE)
            {
                doSendfile(sockData);

                if (sockData->writeComplete)
                {
                    // TODO
                    /*
                    SocketService::socketCallback callback = (SocketService::socketCallback)sockData->readCallback;
                    callback(sockData->aioSocket,
                             sockData->writeUserData,
                             sockData->readBufferPos,
                             error);
                    */
                }
            }
        }
    }

    return true;
}

bool SocketService::poll()
{
    Locker<Condition> mainLocker(_cond);

    if (_isShutdown)
        return false;

    // Fill in the pollfd data
    _pollFdList.clear();

    size_t pollIndex = 0;
    HashMap<int, SockData>::ConstIterator mapIter = _dataMap.iterator();

    while (mapIter.isValid())
    {
        HashMap<int, SockData>::Entry entry = mapIter.value();
        int fd = entry.getKey();
        SockData& sockData = entry.getValue();

        pollfd pollData;
        pollData.fd = fd;
        pollData.events = 0;
        pollData.revents = 0;
        
        if (sockData.readOper != 0 &&
            !sockData.readComplete)
        {
            pollData.events |= POLLIN;
        }

        if (sockData.writeOper != 0 &&
            !sockData.writeComplete)
        {
            pollData.events |= POLLOUT;
        }

        _pollFdList.addBack(pollData);

        pollIndex++;
    }

    // Unlock the main condition and call poll
    mainLocker.unlock();

    int pollRet;
    
    do
    {
        pollRet = ::poll(_pollFdList.data(), _pollFdList.size(), -1);
    }
    while (pollRet == -1 && errno == EINTR);

    if (pollRet == -1)
    {
        // Not much we can do if poll failed
        // TODO: Log
        // TODO: Shutdown?
        return false;
    }

    // If you close a FD passed to poll, prior to the poll returning, you
    // can get errors relating to invalid FDs. To avoid this, a thread desiring
    // to remove a FD can write to the wake pipe, indicate this thread should
    // wait, do the removal and indicate it can stop waiting. This causes a
    // bit of churn, but it's hard to avoid.
    Locker<Condition> pauseLocker(_pauseCond);

    if (_doPause)
    {
        _isPaused = true;

        while (_doPause)
        {
            _pauseCond.wait();
        }

        _isPaused = false;
        _pauseCond.signal(); // TODO: Need?
        return true;
    }

    pauseLocker.unlock();

    // Grab the main condition's lock again
    mainLocker.lock();

    if (_isShutdown)
        return false;

    // Now we walk through our happy set of results
    for (int i = 0; i < pollRet; i++)
    {
        pollfd& pollData = _pollFdList.get(i);

        // Ignore the wakeup pipe
        if (pollData.fd == _wakeupPipe[0])
            continue;

        HashMap<int, SockData>::ConstIterator iter = _dataMap.get(pollData.fd);
        HashMap<int, SockData>::Entry entry = iter.value();

        SockData& sockData = entry.getValue();

        bool queueRead = false;
        bool queueWrite = false;

        // Handle flags indicating a socket issues first
        if ((pollData.revents | POLLERR) != 0)
        {
            // The socket itself is dead somehow

            int errVal = -1;
            socklen_t optLen = sizeof(errVal);
            int optRet = ::getsockopt(pollData.fd, SOL_SOCKET, SO_ERROR, &errVal, &optLen);

            if (optRet == -1)
            {
                // TODO: Not much we can do but log
            }

            if (sockData.readOper != 0)
            {
                sockData.readError = UnixUtil::getError(errVal,
                    "SocketService::?", // TODO: Mapping?
                    "poll");
                queueRead = true;
            }

            if (sockData.writeOper != 0)
            {
                sockData.writeError = UnixUtil::getError(errVal,
                    "SocketService::?", // TODO: Mapping?
                    "poll");
                queueWrite = true;
            }
        }
        else if ((pollData.revents | POLLNVAL) != 0)
        {
            // The FD is somehow bad. This shouldn't be possible, but pass it on to both sides.

            if (sockData.readOper != 0)
            {
                sockData.readError = UnixUtil::getError(EBADF,
                    "SocketService::?", // TODO: Mapping?
                    "poll");
                queueRead = true;
            }

            if (sockData.writeOper != 0)
            {
                sockData.writeError = UnixUtil::getError(EBADF,
                    "SocketService::?", // TODO: Mapping?
                    "poll");
                queueWrite = true;
            }
        }
        else
        {
            // Handle valid output cases
            if ((pollData.revents | POLLHUP) != 0)
            {
                // This indicates the write side of the socket on our side is shutdown

                sockData.writeError = UnixUtil::getError(EBADF,
                    "SocketService::?", // TODO: Mapping?
                    "poll");
                queueWrite = true;
            }
            else if ((pollData.revents | POLLOUT) != 0)
            {
                queueWrite = true;
            }
            
            // Handle valid input cases
            if ((pollData.revents | POLLIN) != 0)
            {
                queueRead = true;
            }
        }

        if (queueRead)
            enqueData(&sockData.readQueueEntry);

        if (queueWrite)
            enqueData(&sockData.writeQueueEntry);
    }

    return true;
}

void SocketService::enqueData(QueueEntry* queueEntry)
{
    if (_readyQueueHead == NULL)
    {
        queueEntry->prev = NULL;
        _readyQueueHead = queueEntry;
        _readyQueueTail = queueEntry;
    }
    else
    {
        _readyQueueTail->next = queueEntry;
    }

    queueEntry->next = NULL;
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
    readOper(0),
    acceptSocket(NULL),
    readCallback(NULL),
    readUserData(NULL),
    readBuffer(NULL),
    readBufferPos(0),
    readBufferLen(0),
    readComplete(false),
    writeOper(0),
    writeCallback(NULL),
    writeUserData(NULL),
    writeBuffer(NULL),
    writeBufferPos(0),
    writeBufferLen(0),
    writeComplete(false),
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

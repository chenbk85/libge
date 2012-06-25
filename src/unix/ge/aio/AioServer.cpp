// AioServer.cpp

#include "ge/aio/AioServer.h"

#include "ge/io/IOException.h"
#include "ge/thread/CurrentThread.h"
#include "ge/util/Locker.h"

#ifdef __linux__
// Use linux aio for files and epoll
#include "gepriv/aio/FileServiceLinuxAio.h"
#include "gepriv/aio/SocketServiceEpoll.h"
#else
// Use blocking file io and poll
#include "gepriv/aio/FileServiceBlocking.h"
#include "gepriv/aio/SocketServicePoll.h"
#endif

#define STATE_NONE 0
#define STATE_STARTED 1
#define STATE_SHUTDOWN 2

// AioWorker ----------------------------------------------------------------

AioServer::AioWorker::AioWorker(AioServer* aioServer) :
    _aioServer(aioServer)
{
}

void AioServer::AioWorker::run() OVERRIDE
{
    CurrentThread::setName("AioWorker");

    bool keepGoing;

    do
    {
        keepGoing = _aioServer->process();
    }
    while (keepGoing);
}

// --------------------------------------------------------------------------

AioServer::AioServer() :
    _fileReadyCount(0),
    _socketReadyCount(0),
    _fileService(NULL),
    _socketService(NULL),
    _state(STATE_NONE)
{
    try
    {
#ifdef __linux__
        // Use linux aio for files and epoll
        _fileService = new FileServiceLinuxAio(this);
        _socketService = new SocketServiceEpoll(this);
#else
        // Use blocking file io and poll
        _fileService = new FileServiceBlocking(this);
        _socketService = new SocketServicePoll(this);
#endif
    }
    catch (...)
    {
        delete _fileService;
        delete _socketService;
        throw;
    }
}

AioServer::~AioServer()
{
    try
    {
        shutdown();
    }
    catch (...)
    {
        // TODO: log
    }

    delete _fileService;
    delete _socketService;
}

void AioServer::startServing(uint32 desiredThreads)
{
    bool started;

    _cond.lock();

    if (_state == STATE_NONE)
    {
        _state = STATE_STARTED;
        started = true;
    }

    _cond.unlock();

    if (!started)
    {
        throw IOException("Cannot restart AioServer");
    }

    // If this throws the destructor will cleanup
    for (uint32 i = 0; i < desiredThreads; i++)
    {
        AioWorker* worker = new AioWorker(this);
        _threads.addBack(worker);

        worker->start();
    }
}

void AioServer::shutdown()
{
    // Check and flip state
    bool doShutdown = false;

    _cond.lock();

    if (_state == STATE_STARTED)
    {
        _state = STATE_SHUTDOWN;
        doShutdown = true;
    }

    _cond.unlock();

    if (!doShutdown)
        return;

    // Shutdown services
    _fileService->shutdown();
    _socketService->shutdown();

    // Join and delete the worker threads
    size_t threadCount = _threads.size();

    for (size_t i = 0; i < threadCount; i++)
    {
        AioWorker* thread = _threads.get(i);
        thread->join();
        delete thread;
    }
}

void AioServer::fileRead(AioFile* aioFile,
                         fileCallback callback,
                         void* userData,
                         uint64 pos,
                         char* buffer,
                         uint32 bufferLen)
{

}

void AioServer::fileWrite(AioFile* aioFile,
                          fileCallback callback,
                          void* userData,
                          uint64 pos,
                          const char* buffer,
                          uint32 bufferLen)
{

}

void AioServer::socketClose(AioSocket* aioSocket,
                            connectCallback callback,
                            void* userData)
{

}

void AioServer::socketAccept(AioSocket* listenSocket,
                             AioSocket* acceptSocket,
                             acceptCallback callback,
                             void* userData)
{

}

void AioServer::socketConnect(AioSocket* aioSocket,
                              connectCallback callback,
                              void* userData,
                              const INetAddress& address,
                              int32 port)
{

}

void AioServer::socketConnect(AioSocket* aioSocket,
                              connectCallback callback,
                              void* userData,
                              const INetAddress& address,
                              int32 port,
                              int32 timeout)
{

}

void AioServer::socketRead(AioSocket* aioSocket,
                           socketCallback callback,
                           void* userData,
                           char* buffer,
                           uint32 bufferLen)
{

}

void AioServer::socketWrite(AioSocket* aioSocket,
                            socketCallback callback,
                            void* userData,
                            const char* buffer,
                            uint32 bufferLen)
{

}

void AioServer::socketSendFile(AioSocket* aioSocket,
                               socketCallback callback,
                               void* userData,
                               AioFile* aioFile,
                               uint64 pos,
                               uint32 writeLen)
{

}

void AioServer::addFile(AioFile* aioFile)
{
    // Mark this server as the owner
    aioFile->_owner = this;

    // Add to the set of files
    _childLock.lock();
    _files.addBack(aioFile);
    _childLock.unlock();
}

void AioServer::addSocket(AioSocket* aioSocket)
{
    // Mark this server as the owner
    aioSocket->_owner = this;

    // Add to the set of files
    _childLock.lock();
    _sockets.addBack(aioSocket);
    _childLock.unlock();
}

void AioServer::dropFile(AioFile* aioFile)
{
    _childLock.lock();

    size_t fileCount = _files.size();
    for (size_t i = 0; i < fileCount; i++)
    {
        AioFile* file = _files.get(i);

        if (file == aioFile)
        {
            _files.remove(i);
            return;
        }
    }

    _childLock.unlock();
}

void AioServer::dropSocket(AioSocket* aioSocket)
{
    _childLock.lock();

    size_t socketCount = _sockets.size();
    for (size_t i = 0; i < socketCount; i++)
    {
        AioSocket* socket = _sockets.get(i);

        if (socket == aioSocket)
        {
            _sockets.remove(i);
            return;
        }
    }

    _childLock.unlock();
}

void AioServer::fileIoReady()
{
    _cond.lock();
    _fileReadyCount++;
    _cond.signal();
    _cond.unlock();
}

void AioServer::socketIoReady()
{
    _cond.lock();
    _socketReadyCount++;
    _cond.signal();
    _cond.unlock();
}

bool AioServer::process()
{
    bool fileReady = false;
    bool socketReady = false;

    Locker<Condition> locker(_cond);

    // Wait until there is some work or is shutdown
    while (_state != STATE_SHUTDOWN &&
           _fileReadyCount == 0 &&
           _socketReadyCount == 0)
    {
        _cond.wait();
    }

    // Arbitrarily favoring socket IO at the moment as it is less likely
    // to block
    if (_socketReadyCount != 0)
    {
        _socketReadyCount--;
        socketReady = true;
    }
    else if (_fileReadyCount != 0)
    {
        _fileReadyCount--;
        fileReady = true;
    }

    locker.unlock();

    // Differ work to the appropriate service
    if (socketReady)
    {
        _socketService->process();
    }
    else if (fileReady)
    {
        _fileService->process();
    }
    else
    {
        return false;
    }

    return true;
}

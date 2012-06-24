// AioServer.cpp

#include "ge/aio/AioServer.h"

#include "ge/io/IOException.h"
#include "ge/thread/CurrentThread.h"

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
        _fileService = new FileServiceLinuxAio();
        _socketService = new SocketServiceEpoll();
#else
        // Use blocking file io and poll
        _fileService = new FileServiceBlocking();
        _socketService = new SocketServicePoll();
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
    uint32 oldState;

    _stateLock.lock();

    oldState = _state;

    if (_state == STATE_NONE)
        _state = STATE_STARTED;

    _stateLock.unlock();

    if (_state != STATE_STARTED)
    {
        throw IOException("Cannot restart AioServer");
    }

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

    _stateLock.lock();

    if (_state == STATE_STARTED)
    {
        _state = STATE_SHUTDOWN;
        doShutdown = true;
    }

    _stateLock.unlock();

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
    _stateLock.lock();
    _files.addBack(aioFile);
    _stateLock.unlock();
}

void AioServer::addSocket(AioSocket* aioSocket)
{
    // Mark this server as the owner
    aioSocket->_owner = this;

    // Add to the set of files
    _stateLock.lock();
    _sockets.addBack(aioSocket);
    _stateLock.unlock();
}

void AioServer::dropFile(AioFile* aioFile)
{
    _stateLock.lock();

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

    _stateLock.unlock();
}

void AioServer::dropSocket(AioSocket* aioSocket)
{
    _stateLock.lock();

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

    _stateLock.unlock();
}

bool AioServer::process()
{
    return true;
}

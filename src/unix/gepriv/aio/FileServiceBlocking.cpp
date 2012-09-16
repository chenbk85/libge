// FileServiceBlocking.cpp

#include "gepriv/aio/FileServiceBlocking.h"

#include "ge/io/IOException.h"
#include "ge/thread/CurrentThread.h"
#include "ge/util/Locker.h"
#include "gepriv/UnixUtil.h"

#include <errno.h>
#include <unistd.h>

FileService::FileService() :
    _isShutdown(false)
{
}

FileService::~FileService()
{
    shutdown();
}

void FileService::startServing(uint32 desiredThreads)
{
    // Create worker threads
    // If this throws we're depending on the destructor for cleanup
    for (uint32 i = 0; i < desiredThreads; i++)
    {
        AioWorker* worker = new AioWorker(this);
        _threads.addBack(worker);

        worker->start();
    }
}

void FileService::shutdown()
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

void FileService::fileRead(AioFile* aioFile,
                           FileService::fileCallback callback,
                           void* userData,
                           uint64 pos,
                           char* buffer,
                           uint32 bufferLen)
{
    QueueData queueData;
    queueData.isRead = true;
    queueData.aioFile = aioFile;
    queueData.callback = callback;
    queueData.userData = userData;
    queueData.pos = pos;
    queueData.buffer = buffer;
    queueData.bufferLen = bufferLen;

    Locker<Condition> locker(_cond);

    if (_isShutdown)
    {
        throw IOException("Cannot submit IO to shutdown FileService");
    }

    _queue.addBack(queueData);
    _cond.signal();

    locker.unlock();
}

void FileService::fileWrite(AioFile* aioFile,
                            FileService::fileCallback callback,
                            void* userData,
                            uint64 pos,
                            const char* buffer,
                            uint32 bufferLen)
{
    QueueData queueData;
    queueData.isRead = false;
    queueData.aioFile = aioFile;
    queueData.callback = callback;
    queueData.userData = userData;
    queueData.pos = pos;
    queueData.buffer = (char*)buffer;
    queueData.bufferLen = bufferLen;

    Locker<Condition> locker(_cond);

    if (_isShutdown)
    {
        throw IOException("Cannot submit IO to shutdown FileService");
    }

    _queue.addBack(queueData);
    _cond.signal();

    locker.unlock();
}

void FileService::dropFile(AioFile* aioFile)
{

}

bool FileService::process()
{
    // Grab some data
    Locker<Condition> locker(_cond);

    while (!_isShutdown &&
           !_queue.isEmpty())
    {
        _cond.wait();
    }

    if (_isShutdown)
        return false;

    QueueData workData = _queue.front();
    _queue.popFront();
    locker.unlock();

    Error error;
    int res;

    if (workData.isRead)
    {
        do
        {
            res = ::pread(workData.aioFile->_fd,
                          workData.buffer,
                          workData.bufferLen,
                          workData.pos);
        } while (res == -1 && errno == EINTR);

        if (res == -1)
        {
            error = UnixUtil::getError(errno,
                                       "pread",
                                       "FileService::fileRead");
        }
    }
    else
    {
        do
        {
            res = ::pwrite(workData.aioFile->_fd,
                           workData.buffer,
                           workData.bufferLen,
                           workData.pos);
        } while (res == -1 && errno == EINTR);

        if (res == -1)
        {
            error = UnixUtil::getError(errno,
                                       "pwrite",
                                       "FileService::fileWrite");
        }
    }

    workData.callback(workData.aioFile,
                      workData.userData,
                      workData.bufferLen,
                      error);
    return true;
}

// Inner Classes ------------------------------------------------------------

FileService::AioWorker::AioWorker(FileService* fileService) :
    _fileService(fileService)
{
}

void FileService::AioWorker::run()
{
    CurrentThread::setName("FileService Worker");

    bool keepGoing = true;

    while (keepGoing)
    {
        keepGoing = _fileService->process();
    }
}

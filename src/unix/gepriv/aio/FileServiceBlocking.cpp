// FileServiceBlocking.cpp

#include "gepriv/aio/FileServiceBlocking.h"

#include "ge/util/Locker.h"
#include "gepriv/UnixUtil.h"

#include <errno.h>
#include <unistd.h>

FileServiceBlocking::FileServiceBlocking(AioServer* aioServer) :
    _aioServer(aioServer)
{
}

FileServiceBlocking::~FileServiceBlocking()
{

}

void FileServiceBlocking::process()
{
    // Grab some data
    Locker<Mutex> locker(_lock);
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
                                       "AioServer::fileRead");
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
                                       "AioServer::fileWrite");
        }
    }

    workData.callback(workData.aioFile,
                      workData.userData,
                      workData.bufferLen,
                      error);
}

void FileServiceBlocking::shutdown()
{
    // Nothing to do
}

void FileServiceBlocking::submitRead(AioFile* aioFile,
                                     AioServer::fileCallback callback,
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

    Locker<Mutex> locker(_lock);
    _queue.addBack(queueData);
    locker.unlock();

    _aioServer->fileIoReady();
}

void FileServiceBlocking::submitWrite(AioFile* aioFile,
                                      AioServer::fileCallback callback,
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

    Locker<Mutex> locker(_lock);
    _queue.addBack(queueData);
    locker.unlock();

    _aioServer->fileIoReady();
}

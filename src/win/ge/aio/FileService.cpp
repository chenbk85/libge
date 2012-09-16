// FileService.cpp

#include "ge/aio/FileService.h"

#include "ge/io/IOException.h"
#include "ge/thread/CurrentThread.h"
#include "ge/thread/Thread.h"
#include "ge/SystemException.h"
#include "gepriv/WinUtil.h"

#include <Windows.h>


// Key passed for Windows generated completion queue post
#define COMPLETION_KEY_NATIVE (ULONG_PTR)1

// Key passed for FileService generated completion queue post
#define COMPLETION_KEY_SERVER (ULONG_PTR)2

// Size of addresses that ACCEPTEX expects It requires 16 bytes more than
// maximum address size for both local and remote addresses.
#define ACCEPTEX_ADDRESS_SIZE (sizeof(sockaddr_in6) + 16)

// State values for the server state
#define STATE_NONE 0
#define STATE_STARTED 1
#define STATE_SHUTDOWN 2

// Global for assigning ids to servers
static LONG volatile g_nextServerId;

enum Overlapped_Op
{
    OP_READ_FILE,
    OP_WRITE_FILE,
    OP_SHUTDOWN
};

/*
 * Structure beginning with an OVERLAPPED that contains extra data. We
 * pass this to get extra data when an IO operation completes.
 */
struct OVERLAPPED_EX
{
    OVERLAPPED        overlapped;
    Overlapped_Op     opCode;

    // Common fields
    AioFile*          aioFile;
    char*             buffer;
    uint32            bufferSize;
    uint64            offset;
    FileService::fileCallback callback;
    void*             userData;
};

/*
 * Checks if the passed error indicates that something was queued to an
 * IO completion port.
 */
static
bool completionQueued(int winErr)
{
    // Before Windows Vista, we can't avoid the queing of IO on success.
#if (_WIN32_WINNT < 0x0600)
    if (winErr == ERROR_SUCCESS)
        return true;
#endif

    return (winErr == ERROR_IO_PENDING);
}


FileService::FileService() :
    _completionPort(NULL),
    _state(STATE_NONE),
    _pending(0)
{
}

FileService::~FileService()
{
    shutdown();
}

void FileService::startServing(uint32 desiredThreads)
{
    LONG oldState = ::InterlockedCompareExchange(&_state, STATE_STARTED, STATE_NONE);

    if (oldState != STATE_NONE)
    {
        throw IOException("Cannot restart FileService");
    }

    // Set up the IO completion port
    _completionPort =
        ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, // Port handle, indicates new competion port
                                 NULL, // Value of existing completion port
                                 COMPLETION_KEY_NATIVE, // Key passed to callbacks
                                 0); // Desired concurrency value (default for now)

    if (_completionPort == NULL)
    {
        Error err = WinUtil::getError(::GetLastError(),
            "CreateIoCompletionPort",
            "FileService::startServing");
        throw IOException(err);
    }

    // Create worker threads

    for (uint32 i = 0; i < desiredThreads; i++)
    {
        AioWorker* worker = new AioWorker(this);
        _threads.addBack(worker);

        worker->start();
    }
}

void FileService::shutdown()
{
    LONG oldState = ::InterlockedExchange(&_state, STATE_SHUTDOWN);

    if (oldState == STATE_NONE ||
        oldState == STATE_SHUTDOWN)
    {
        return;
    }

    // To shutdown in Vista we could just cancel IO with CancelIoEx and
    // close the completion port. It's harder in XP where threads block
    // forever if you close the completion port handle and which only
    // has CancelIo which cancels IO for the calling thread.
    //
    // We wake each thread with an OVERLAPPED with a special OP_SHUTDOWN
    // opCode which tells the worker to cancel all IO and exit. This
    // thread then polls until the completion queue is emptied and we've
    // freed all the OVERLAPPED_EX structs.

    // Mark as in the midst of shutting down

    // For compatability with XP, where no error is generated when waiting
    // on a completion port that is closed, we manually wake all threads.
    OVERLAPPED_EX overlappedEx;
    overlappedEx.opCode = OP_SHUTDOWN;

    size_t workerCount = _threads.size();
    for (size_t i = 0; i < workerCount; i++)
    {
        BOOL res = ::PostQueuedCompletionStatus(_completionPort,
            0,
            COMPLETION_KEY_SERVER,
            &overlappedEx.overlapped);

        // May be better just to close the port on failure and log
        if (!res)
        {
            Error err = WinUtil::getError(::GetLastError(),
                "PostQueuedCompletionStatus",
                "FileService::shutdown");

            throw IOException(err);
        }
    }

    // Join the threads
    for (size_t i = 0; i < workerCount; i++)
    {
        AioWorker* worker = _threads.get(i);
        worker->join();
        delete worker;
    }

    // Empty the completion port
    // Note that we are accessing an atomic variable directly, but the other
    // threads have all been joined.
    while (_pending > 0)
    {
        OVERLAPPED_EX* drainedOverlapped;
        DWORD bytesTransfered;
        ULONG_PTR completionKey;

        BOOL res = ::GetQueuedCompletionStatus(_completionPort, // Completion port,
                                               &bytesTransfered,
                                               &completionKey,
                                               (LPOVERLAPPED*)&drainedOverlapped,
                                               INFINITE);
     
        if (res)
        {
            ::CloseHandle(drainedOverlapped->overlapped.hEvent);
            delete drainedOverlapped;
        }
        else
        {
            int winErr = ::GetLastError();

            if (winErr == ERROR_OPERATION_ABORTED)
            {
                // This is the expected error code for canceled IO
                ::CloseHandle(drainedOverlapped->overlapped.hEvent);
                delete drainedOverlapped;
            }
            else
            {
                Error err = WinUtil::getError(winErr,
                    "GetQueuedCompletionStatus",
                    "FileService::shutdown");

                // TODO: Log
            }
        }

        _pending--;
    }

     // Close the io completion port
    ::CloseHandle(_completionPort);
    _completionPort = NULL;
}

void FileService::fileRead(AioFile* aioFile,
                         fileCallback callback,
                         void* userData,
                         uint64 pos,
                         char* buffer,
                         uint32 bufferLen)
{
    if (aioFile->_handle == INVALID_HANDLE_VALUE)
    {
        throw IOException("Cannot read from closed file");
    }

    if (_state != STATE_STARTED)
    {
        throw IOException("FileService not running");
    }

    // Associate the file with this server if have not already done so
    if (aioFile->_owner == NULL)
    {
        Error err = addFile(aioFile, "fileRead");

        if (err.isSet())
            throw IOException(err);
    }
    else if (aioFile->_owner != this)
    {
        throw IOException("Called FileService::fileRead call with AioSocket "
            "owned by another FileService");
    }

    // Create an OVERLAPPED_EX with data for ReadFile
    OVERLAPPED_EX* overlappedEx = new OVERLAPPED_EX();

    overlappedEx->overlapped.hEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
    overlappedEx->overlapped.OffsetHigh = (uint32)(pos >> 32);
    overlappedEx->overlapped.Offset = (uint32)pos;
    overlappedEx->opCode = OP_READ_FILE;
    overlappedEx->callback = callback;
    overlappedEx->aioFile = aioFile;
    overlappedEx->buffer = buffer;
    overlappedEx->bufferSize = bufferLen;
    overlappedEx->userData = userData;

    ::InterlockedIncrement(&_pending);

    // Add to the completion queue
    BOOL res = ::PostQueuedCompletionStatus(_completionPort,
        0,
        COMPLETION_KEY_SERVER,
        &overlappedEx->overlapped);

    if (!res)
    {
        delete overlappedEx;
        ::InterlockedDecrement(&_pending);

        Error err = WinUtil::getError(::GetLastError(),
            "PostQueuedCompletionStatus",
            "FileService::fileRead");

        throw IOException(err);
    }
}

void FileService::fileWrite(AioFile* aioFile,
                           fileCallback callback,
                           void* userData,
                           uint64 pos,
                           const char* buffer,
                           uint32 bufferLen)
{
    if (aioFile->_handle == INVALID_HANDLE_VALUE)
    {
        throw IOException("Cannot write to closed file");
    }

    if (_state != STATE_STARTED)
    {
        throw IOException("FileService not running");
    }

    // Associate the file with this server if have not already done so
    if (aioFile->_owner == NULL)
    {
        Error err = addFile(aioFile, "fileWrite");

        if (err.isSet())
            throw IOException(err);
    }
    else if (aioFile->_owner != this)
    {
        throw IOException("Called FileService::fileWrite call with AioSocket "
            "owned by another FileService");
    }

    // Create an OVERLAPPED_EX with data for WriteFile
    OVERLAPPED_EX* overlappedEx = new OVERLAPPED_EX();

    overlappedEx->overlapped.hEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
    overlappedEx->overlapped.OffsetHigh = (uint32)(pos >> 32);
    overlappedEx->overlapped.Offset = (uint32)pos;
    overlappedEx->callback = callback;
    overlappedEx->opCode = OP_WRITE_FILE;
    overlappedEx->aioFile = aioFile;
    overlappedEx->buffer = (char*)buffer;
    overlappedEx->bufferSize = bufferLen;
    overlappedEx->userData = userData;

    ::InterlockedIncrement(&_pending);

    // Add to the completion queue
    BOOL res = ::PostQueuedCompletionStatus(_completionPort,
        0,
        COMPLETION_KEY_SERVER,
        &overlappedEx->overlapped);

    if (!res)
    {
        delete overlappedEx;
        ::InterlockedDecrement(&_pending);

        Error err = WinUtil::getError(::GetLastError(),
            "PostQueuedCompletionStatus",
            "FileService::fileWrite");

        throw IOException(err);
    }
}

Error FileService::addFile(AioFile* aioFile,
                         const char* context)
{
    // Add the file to the completion port
    HANDLE res = ::CreateIoCompletionPort(aioFile->_handle, 
                                          _completionPort,
                                          (ULONG_PTR)NULL,
                                          0);

    if (res == NULL)
    {
        return WinUtil::getError(::GetLastError(),
                                 "CreateIoCompletionPort",
                                 context);
    }

    // Mark this server as the owner
    aioFile->_owner = this;

    // Add to the set of files
    _lock.lock();
    _files.addBack(aioFile);
    _lock.unlock();

    return Error();
}

void FileService::dropFile(AioFile* aioFile)
{
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
}

bool FileService::process()
{
    OVERLAPPED_EX* overlappedEx = NULL;
    DWORD bytesTransfered = 0;
    ULONG_PTR completionKey;
    uint32 completionValue;

    FileService::fileCallback userFileCallback = NULL;

    DWORD flags = 0;

    Error error;
    BOOL bRet;
    int winErr = 0;

    // Attempt to de-queue one of the OVERLAPPED objects we queued manually,
    // or through calling a win function supporting overlapped operations.
    bRet = ::GetQueuedCompletionStatus(_completionPort, // Completion port,
                                       &bytesTransfered,
                                       &completionKey,
                                       (LPOVERLAPPED*)&overlappedEx,
                                       INFINITE);
     
    if (!bRet)
    {
        winErr = ::GetLastError();

        // If the overlapped returned is NULL, then the function itself
        // failed (rather than the function that queud the OVERLAPPED)
        if (overlappedEx == NULL)
        {
            // If it reports the wait was abandonned, it means the port was closed.
            // Just exit the worker thread.
            if (winErr == ERROR_ABANDONED_WAIT_0)
                return false;

            error = WinUtil::getError(winErr,
                "GetQueuedCompletionStatus",
                "FileService::process");

            return false;
        }

        // Otherwise the de-queue succeeded, fall through
    }

    // If we get an opCode of OP_SHUTDOWN, we attempt to cancel all IO and
    // then exit the thread. CancelIo works on a per-thread basis. This logic
    // Can be simplified with CancelIoEx, but it's Vista+.
    if (overlappedEx->opCode == OP_SHUTDOWN)
    {
        _lock.lock();

        size_t filesSize = _files.size();
        for (size_t i = 0; i < filesSize; i++)
        {
            AioFile* aioFile = _files.get(i);
            bRet = ::CancelIo(aioFile->_handle);

            if (!bRet)
            {
                // TODO: Log?
            }
        }

        _lock.unlock();
        return false;
    }

    // If the de-queing didn't fail, and it wasn't a shutdown message,
    // decrement the number of pending items.
    ::InterlockedDecrement(&_pending);

    completionValue = (uint32)completionKey;

    // If the completion key indicates it's a message we generated, trigged
    // a function call as defined by its opCode value.
    if (completionValue == COMPLETION_KEY_SERVER)
    {
        switch (overlappedEx->opCode)
        {
            case OP_READ_FILE:
                bRet = ::ReadFile(overlappedEx->aioFile->_handle,
                                  overlappedEx->buffer,
                                  overlappedEx->bufferSize,
                                  &bytesTransfered,
                                  &overlappedEx->overlapped);

                // TRUE indicates immediate success, FALSE means failure or queued.
                if (bRet != TRUE)
                {
                    winErr = ::GetLastError();

                    // The ERROR_HANDLE_EOF just means we hit end of file and
                    // read nothing
                    if (winErr != ERROR_IO_PENDING &&
                        winErr != ERROR_HANDLE_EOF)
                    {
                        error = WinUtil::getError(winErr,
                            "ReadFile",
                            "FileService::fileRead");
                    }
                }

                // Trigger callback if completed immediately or failed
                if (!completionQueued(winErr))
                {
                    userFileCallback = (FileService::fileCallback)overlappedEx->callback;
                    userFileCallback(overlappedEx->aioFile,
                                     overlappedEx->userData,
                                     bytesTransfered,
                                     error);
                }

                break;
            case OP_WRITE_FILE:
                bRet = ::WriteFile(overlappedEx->aioFile->_handle,
                                   overlappedEx->buffer,
                                   overlappedEx->bufferSize,
                                   &bytesTransfered,
                                   &overlappedEx->overlapped);

                // TRUE indicates immediate success, FALSE means failure or queued.
                if (bRet != TRUE)
                {
                    winErr = ::GetLastError();

                    if (winErr != ERROR_IO_PENDING)
                    {
                        error = WinUtil::getError(winErr,
                            "WriteFile",
                            "FileService::fileWrite");
                    }
                }

                // Trigger callback if completed immediately or failed
                if (!completionQueued(winErr))
                {
                    userFileCallback = (FileService::fileCallback)overlappedEx->callback;
                    userFileCallback(overlappedEx->aioFile,
                                     overlappedEx->userData,
                                     bytesTransfered,
                                     error);
                }

                break;
        }
    }
    else // if (completionValue == COMPLETION_KEY_NATIVE)
    {
        // We de-queued an operation that either failed or succeeded. If it
        // failed with an error, winErr will be non-zero.
        switch (overlappedEx->opCode)
        {
            case OP_READ_FILE:
                // The ERROR_HANDLE_EOF just means we hit end of file
                if (winErr != ERROR_SUCCESS &&
                    winErr != ERROR_HANDLE_EOF)
                {
                    error = WinUtil::getError(winErr,
                            "ReadFile",
                            "FileService::fileRead");
                }

                userFileCallback = (FileService::fileCallback)overlappedEx->callback;
                userFileCallback(overlappedEx->aioFile,
                                    overlappedEx->userData,
                                    0,
                                    error);

            case OP_WRITE_FILE:
                if (winErr != ERROR_SUCCESS)
                {
                    error = WinUtil::getError(winErr,
                            "WriteFile",
                            "FileService::fileWrite");
                }

                userFileCallback = (FileService::fileCallback)overlappedEx->callback;
                userFileCallback(overlappedEx->aioFile,
                                 overlappedEx->userData,
                                 bytesTransfered,
                                 error);
                break;
        }

        if (overlappedEx->overlapped.hEvent)
            ::CloseHandle(overlappedEx->overlapped.hEvent);
        delete overlappedEx;
    }

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

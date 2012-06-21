// AioServer.cpp

#include "ge/aio/AioServer.h"

#include "ge/io/IOException.h"
#include "ge/thread/CurrentThread.h"
#include "ge/thread/Thread.h"
#include "ge/SystemException.h"
#include "gepriv/WinUtil.h"

#define _WINSOCKAPI_
#include <Windows.h>
#include <Winsock2.h>
#include <Mstcpip.h>
#include <Mswsock.h>
#include <process.h>
#include <ws2tcpip.h>

/*
 * Some of the IOCP documentation is out of date. The following rule does seem
 * to hold true. Sockets created with AcceptEx can only be passed to the
 * following functions:
 *
 * ReadFile
 * WriteFile
 * send
 * WSASend
 * recv
 * WSARecv
 * TransmitFile
 * closesocket
 * setsockopt (only for SO_UPDATE_ACCEPT_CONTEXT)
 *
 * http://msdn.microsoft.com/en-us/library/windows/desktop/ms737524%28v=vs.85%29.aspx
 *
 * Some general IOCP advice here:
 * http://support.microsoft.com/default.aspx?scid=kb;en-us;Q192800
 */

// Key passed for Windows generated completion queue post
#define COMPLETION_KEY_NATIVE (ULONG_PTR)1

// Key passed for AioServer generated completion queue post
#define COMPLETION_KEY_SERVER (ULONG_PTR)2

// Size of addresses that ACCEPTEX expects It requires 16 bytes more than
// maximum address size for both local and remote addresses.
#define ACCEPTEX_ADDRESS_SIZE (sizeof(sockaddr_in6) + 16)

// State values for the server state
#define STATE_NONE 0
#define STATE_STARTED 1
#define STATE_SHUTDOWN 2

// Global function pointers to extension functions
LPFN_ACCEPTEX acceptExPtr = NULL;
LPFN_GETACCEPTEXSOCKADDRS getAcceptExSockaddrsPtr = NULL;
LPFN_CONNECTEX connectExPtr = NULL;
LPFN_DISCONNECTEX disconnectExPtr = NULL;
LPFN_TRANSMITFILE transmitFilePtr = NULL;

// GUIDs required to access the above extensions
GUID acceptExGUID = WSAID_ACCEPTEX;
GUID getAcceptExSockaddrsGUID = WSAID_GETACCEPTEXSOCKADDRS;
GUID connectExGUID = WSAID_CONNECTEX;
GUID disconnectExGUID = WSAID_DISCONNECTEX;
GUID transmitFileGUID = WSAID_TRANSMITFILE;

// Global for assigning ids to servers
LONG volatile g_nextServerId;

enum Overlapped_Op
{
    OP_READ_FILE,
    OP_WRITE_FILE,
    OP_ACCEPT,
    OP_CONNECT,
    OP_DISCONNECT,
    OP_TRANSMIT_FILE,
    OP_RECV,
    OP_SEND,
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
    AioSocket*        aioSocket;
    char*             buffer;
    uint32            bufferSize;
    uint64            offset;
    void*             callback;
    void*             userData;

    // Accept only fields
    AioSocket*        acceptedSocket;
    char              addressBuffer[ACCEPTEX_ADDRESS_SIZE * 2];

    // Connect only fields
    INetAddress       address;
    uint32            port;
};

/*
 * Gets an IO extension based on a GUID. Windows demands you get the
 * extended IO functions through the WSAIoctl() interface, but it
 * should always succeed for basic TCP/IP as the functions are directly
 * exported from a DLL.
 */
static
void* getExtension(SOCKET sock,
                   void* guid,
                   const char* guidName)
{
    void *ptr = NULL;
    DWORD bytes=0;
    int res = ::WSAIoctl(sock,
                         SIO_GET_EXTENSION_FUNCTION_POINTER,
                         guid,
                         sizeof(GUID),
                         &ptr,
                         sizeof(ptr),
                         &bytes,
                         NULL,
                         NULL);

    if (res == SOCKET_ERROR)
    {
        throw SystemException(String("WSAIoctl() failed when getting ") +
            guidName + " extension");
    }

    return ptr;
}

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

    return (winErr == WSA_IO_PENDING);
}

/*
 * Configures the socket and extracts address data when an accept succeeds.
 */
static
Error handleAcceptSuccess(OVERLAPPED_EX* overlappedEx,
                          SOCKET listenSocket,
                          SOCKET acceptedSocket)
{
    sockaddr* localAddrPtr;
    INT localAddrLen;
    sockaddr* remoteAddrPtr;
    INT remoteAddrLen;

    // A socket accepted with AcceptEx is in default state until you
    // force it to apply previous options with SO_UPDATE_CONNECT_CONTEXT
    int iRet = ::setsockopt(acceptedSocket,
                            SOL_SOCKET,
                            SO_UPDATE_ACCEPT_CONTEXT, 
                            (char*)&listenSocket,
                            sizeof(SOCKET));

    if (iRet == 0)
    {
        // Extract local and remote addresses
        getAcceptExSockaddrsPtr(overlappedEx->addressBuffer,
                                0,
                                ACCEPTEX_ADDRESS_SIZE,
                                ACCEPTEX_ADDRESS_SIZE,
                                &localAddrPtr,
                                &localAddrLen,
                                &remoteAddrPtr,
                                &remoteAddrLen);

        // TODO: Use values
    }
    else
    {
        return WinUtil::getError(::WSAGetLastError(),
            "setsockopt",
            "AioServer::socketConnect");
    }

    return Error();
}

/*
 * Configures the socket when a connect succeeds.
 */
static
Error handleConnectSuccess(OVERLAPPED_EX* overlappedEx,
                           SOCKET connectedSocket)
{
    // A socket connected with ConnectEx is in default state until you
    // force it to apply previous options with SO_UPDATE_CONNECT_CONTEXT
    int iRet = ::setsockopt(connectedSocket,
                            SOL_SOCKET,
                            SO_UPDATE_CONNECT_CONTEXT,
                            NULL,
                            0);

    if (iRet != 0)
    {
        return WinUtil::getError(::WSAGetLastError(),
            "setsockopt",
            "AioServer::socketConnect");
    }

    // TODO: Fill in local/peer address info or somehow mark as connected
    // so it can be fetched later

    return Error();
}

class AioWorker : public Thread
{
public:
    AioWorker(AioServer* aioServer) :
        _aioServer(aioServer)
    {
    }

    void run() OVERRIDE
    {
        CurrentThread::setName("AioWorker");

        bool keepGoing = true;

        while (keepGoing)
        {
            keepGoing = _aioServer->process();
        }
    }

private:
    AioServer* _aioServer;
};

AioServer::AioServer() :
    _completionPort(NULL),
    _state(STATE_NONE),
    _pending(0)
{
}

AioServer::~AioServer()
{
    shutdown();
}

Error AioServer::startServing(uint32 desiredThreads)
{
    LONG oldState = ::InterlockedCompareExchange(&_state, STATE_STARTED, STATE_NONE);

    if (oldState != STATE_NONE)
    {
        throw IOException("Cannot restart AioServer");
    }

    // Set up the IO completion port
    _completionPort =
        ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, // Port handle, indicates new competion port
                                 NULL, // Value of existing completion port
                                 COMPLETION_KEY_NATIVE, // Key passed to callbacks
                                 0); // Desired concurrency value (default for now)

    if (_completionPort == NULL)
    {
        return WinUtil::getError(::GetLastError(),
            "CreateIoCompletionPort",
            "AioServer::startServing");
    }

    // Create worker threads

    for (uint32 i = 0; i < desiredThreads; i++)
    {
        AioWorker* worker = new AioWorker(this);
        _threads.addBack(worker);

        worker->start();
    }

    return Error();
}

void AioServer::shutdown()
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
                "AioServer::fileRead");

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
                    "AioServer::shutdown");

                // TODO: Log
            }
        }

        _pending--;
    }

     // Close the io completion port
    ::CloseHandle(_completionPort);
    _completionPort = NULL;
}

void AioServer::fileRead(AioFile* aioFile,
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
        throw IOException("AioServer not running");
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
        throw IOException("Called AioServer::fileRead call with AioSocket "
            "owned by another AioServer");
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
            "AioServer::fileRead");

        throw IOException(err);
    }
}

void AioServer::fileWrite(AioFile* aioFile,
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
        throw IOException("AioServer not running");
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
        throw IOException("Called AioServer::fileWrite call with AioSocket "
            "owned by another AioServer");
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
            "AioServer::fileWrite");

        throw IOException(err);
    }
}

void AioServer::socketClose(AioSocket* aioSocket,
                            connectCallback callback,
                            void* userData)
{
    int err = 0;

    if (aioSocket->_winSocket == INVALID_SOCKET)
    {
        throw IOException("Cannot close on uninitialized or closed socket");
    }

    if (_state != STATE_STARTED)
    {
        throw IOException("AioServer not running");
    }

    // Get the DisconnectEx extension if needed
    if (disconnectExPtr == NULL)
    {
        disconnectExPtr = (LPFN_DISCONNECTEX)
            getExtension(aioSocket->_winSocket,
                         &disconnectExGUID,
                         "WSAID_DISCONNECTEX");
    }

    // Validate the socket
    if (aioSocket->_owner == NULL)
    {
        throw IOException("Called AioServer::socketClose call with "
            "AioSocket that is not connected");
    }
    else if (aioSocket->_owner != this)
    {
        throw IOException("Called AioServer::socketClose call with "
            "AioSocket owned by another AioServer");
    }

    // Create an OVERLAPPED_EX with data for DisconnectEx
    OVERLAPPED_EX* overlappedEx = new OVERLAPPED_EX();

    overlappedEx->overlapped.hEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
    overlappedEx->opCode = OP_DISCONNECT;
    overlappedEx->callback = callback;
    overlappedEx->aioSocket = aioSocket;
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

        Error err = WinUtil::getError(::WSAGetLastError(),
            "PostQueuedCompletionStatus",
            "AioServer::socketClose");

        throw IOException(err);
    }
}

void AioServer::socketAccept(AioSocket* listenSocket,
                             AioSocket* acceptingSocket,
                             acceptCallback callback,
                             void* userData)
{
    int err = 0;

    if (listenSocket->_winSocket == INVALID_SOCKET)
    {
        throw IOException("Cannot accept on uninitialized or closed socket");
    }

    if (_state != STATE_STARTED)
    {
        throw IOException("AioServer not running");
    }

    // Get the AcceptEx extension if needed
    if (acceptExPtr == NULL)
    {
        acceptExPtr = (LPFN_ACCEPTEX)
            getExtension(listenSocket->_winSocket,
                         &acceptExGUID,
                         "WSAID_ACCEPTEX");
    }

    // Get the GetAcceptExSockaddrs extension if needed
    if (getAcceptExSockaddrsPtr == NULL)
    {
        getAcceptExSockaddrsPtr = (LPFN_GETACCEPTEXSOCKADDRS)
            getExtension(listenSocket->_winSocket,
                         &getAcceptExSockaddrsGUID,
                         "WSAID_GETACCEPTEXSOCKADDRS");
    }

    // Associate the accepting socket
    if (listenSocket->_owner == NULL)
    {
        Error err = addSocket(listenSocket, "socketAccept");

        if (err.isSet())
            throw IOException(err);
    }
    else if (listenSocket->_owner != this)
    {
        throw IOException("Called AioServer::socketAccept call with "
            "AioSocket owned by another AioServer");
    }

    // Initialize the acceptingSocket socket if needed
    if (acceptingSocket->_winSocket == INVALID_SOCKET)
    {
        acceptingSocket->init(listenSocket->_family);
    }

    // Associate the accepting socket
    if (acceptingSocket->_owner == NULL)
    {
        Error err = addSocket(acceptingSocket, "socketAccept");

        if (err.isSet())
            throw IOException(err);
    }
    else if (acceptingSocket->_owner != this)
    {
        throw IOException("Called AioServer::socketAccept call with "
            "AioSocket owned by another AioServer");
    }

    // Create an OVERLAPPED_EX with data for AcceptEx
    OVERLAPPED_EX* overlappedEx = new OVERLAPPED_EX();

    overlappedEx->overlapped.hEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
    overlappedEx->opCode = OP_ACCEPT;
    overlappedEx->callback = callback;
    overlappedEx->aioSocket = listenSocket;
    overlappedEx->acceptedSocket = acceptingSocket;
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

        Error err = WinUtil::getError(::WSAGetLastError(),
            "PostQueuedCompletionStatus",
            "AioServer::socketAccept");

        throw IOException(err);
    }
}

void AioServer::socketConnect(AioSocket* aioSocket,
                              connectCallback callback,
                              void* userData,
                              const INetAddress& address,
                              int32 port)
{
    socketConnect(aioSocket,
                  callback,
                  userData,
                  address,
                  port,
                  0);
}

void AioServer::socketConnect(AioSocket* aioSocket,
                              connectCallback callback,
                              void* userData,
                              const INetAddress& address,
                              int32 port,
                              int32 timeout)
{
    if (_state != STATE_STARTED)
    {
        throw IOException("AioServer not running");
    }

    // Initialize the socket if haven't already done so
    if (aioSocket->_winSocket == INVALID_SOCKET)
    {
        aioSocket->init(address.getFamily());
    }

    // Get the ConnectEx extension if needed
    if (connectExPtr == NULL)
    {
        connectExPtr = (LPFN_CONNECTEX)
            getExtension(aioSocket->_winSocket,
                         &connectExGUID,
                         "WSAID_CONNECTEX");
    }

    // Validate the socket
    if (aioSocket->_owner == NULL)
    {
        Error err = addSocket(aioSocket, "socketConnect");

        if (err.isSet())
            throw IOException(err);
    }
    else if (aioSocket->_owner != this)
    {
        throw IOException("Called AioServer::socketConnect call with "
            "AioSocket owned by another AioServer");
    }

    // ConnectEx requires that the socket be bound. Bind it.
    // TODO: How to handle case where already bound??
    aioSocket->bind(INetAddress::getAddrAny(address.getFamily()), 0);

    // Create an OVERLAPPED_EX with data for WSARecvMsg
    OVERLAPPED_EX* overlappedEx = new OVERLAPPED_EX();

    overlappedEx->overlapped.hEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
    overlappedEx->opCode = OP_CONNECT;
    overlappedEx->address = address;
    overlappedEx->port = port;
    overlappedEx->callback = callback;
    overlappedEx->aioSocket = aioSocket;
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

        Error err = WinUtil::getError(::WSAGetLastError(),
            "PostQueuedCompletionStatus",
            "AioServer::socketConnect");

        throw IOException(err);
    }
}

void AioServer::socketRead(AioSocket* aioSocket,
                           socketCallback callback,
                           void* userData,
                           char* buffer,
                           uint32 bufferLen)
{
    int err = 0;

    if (aioSocket->_winSocket == INVALID_SOCKET)
    {
        throw IOException("Cannot read from an unconnected socket");
    }

    if (_state != STATE_STARTED)
    {
        throw IOException("AioServer not running");
    }

    // Associate the socket and file with this server if have not
    // already done so
    if (aioSocket->_owner == NULL)
    {
        Error err = addSocket(aioSocket, "socketRead");

        if (err.isSet())
            throw IOException(err);
    }
    else if (aioSocket->_owner != this)
    {
        throw IOException("Called AioServer::socketRead call with "
            "AioSocket owned by another AioServer");
    }

    // Create an OVERLAPPED_EX with data for WSARecv
    OVERLAPPED_EX* overlappedEx = new OVERLAPPED_EX();

    overlappedEx->overlapped.hEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
    overlappedEx->opCode = OP_RECV;
    overlappedEx->callback = callback;
    overlappedEx->aioSocket = aioSocket;
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

        Error err = WinUtil::getError(::WSAGetLastError(),
            "PostQueuedCompletionStatus",
            "AioServer::socketRead");

        throw IOException(err);
    }
}

void AioServer::socketWrite(AioSocket* aioSocket,
                            socketCallback callback,
                            void* userData,
                            const char* buffer,
                            uint32 bufferLen)
{
    int err = 0;

    if (aioSocket->_winSocket == INVALID_SOCKET)
    {
        throw IOException("Cannot write to an unconnected socket");
    }

    if (_state != STATE_STARTED)
    {
        throw IOException("AioServer not running");
    }

    // Associate the socket and file with this server if have not
    // already done so
    if (aioSocket->_owner == NULL)
    {
        Error err = addSocket(aioSocket, "socketWrite");

        if (err.isSet())
            throw IOException(err);
    }
    else if (aioSocket->_owner != this)
    {
        throw IOException("Called AioServer::socketWrite call with "
            "AioSocket owned by another AioServer");
    }

    // Create an OVERLAPPED_EX with data for WSASend
    OVERLAPPED_EX* overlappedEx = new OVERLAPPED_EX();

    overlappedEx->overlapped.hEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
    overlappedEx->opCode = OP_SEND;
    overlappedEx->callback = callback;
    overlappedEx->aioSocket = aioSocket;
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

        Error err = WinUtil::getError(::WSAGetLastError(),
            "PostQueuedCompletionStatus",
            "AioServer::socketWrite");

        throw IOException(err);
    }
}

void AioServer::socketSendFile(AioSocket* aioSocket,
                               socketCallback callback,
                               void* userData,
                               AioFile* aioFile,
                               uint64 pos,
                               uint32 len)
{
    if (_state != STATE_STARTED)
    {
        throw IOException("AioServer not running");
    }

    if (aioFile->_handle == INVALID_HANDLE_VALUE)
    {
        throw IOException("Cannot read from a closed file");
    }

    if (aioSocket->_winSocket == INVALID_SOCKET)
    {
        throw IOException("Cannot write to an unconnected socket");
    }

    // Get the TransmitFile extension if needed
    if (transmitFilePtr == NULL)
    {
        transmitFilePtr = (LPFN_TRANSMITFILE)
            getExtension(aioSocket->_winSocket,
                         &transmitFileGUID,
                         "WSAID_TRANSMITFILE");
    }

    // Associate the socket and file with this server if have not
    // already done so
    if (aioSocket->_owner == NULL)
    {
        Error err = addSocket(aioSocket, "socketSendFile");

        if (err.isSet())
            throw IOException(err);
    }
    else if (aioSocket->_owner != this)
    {
        throw IOException("Called AioServer::socketSendFile call with "
            "AioSocket owned by another AioServer");
    }

    if (aioFile->_owner == NULL)
    {
        Error err = addFile(aioFile, "socketConnect");

        if (err.isSet())
            throw IOException(err);
    }
    else if (aioFile->_owner != this)
    {
        throw IOException("Called AioServer::socketSendFile call with "
            "AioFile owned by another AioServer");
    }

    // Create an OVERLAPPED_EX with data for TransmitFile
    OVERLAPPED_EX* overlappedEx = new OVERLAPPED_EX();

    overlappedEx->overlapped.hEvent = ::CreateEventW(NULL, FALSE, FALSE, NULL);
    overlappedEx->overlapped.OffsetHigh = (uint32)(pos >> 32);
    overlappedEx->overlapped.Offset = (uint32)pos;
    overlappedEx->opCode = OP_TRANSMIT_FILE;
    overlappedEx->bufferSize = len; // Using field for number of bytes to write
    overlappedEx->callback = callback;
    overlappedEx->aioSocket = aioSocket;
    overlappedEx->aioFile = aioFile;
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

        Error err = WinUtil::getError(::WSAGetLastError(),
            "PostQueuedCompletionStatus",
            "AioServer::socketSendFile");

        throw IOException(err);
    }
}

Error AioServer::addFile(AioFile* aioFile,
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

Error AioServer::addSocket(AioSocket* aioSocket,
                           const char* context)
{
    // Add the socket to the completion port
    HANDLE res = ::CreateIoCompletionPort((HANDLE)aioSocket->_winSocket,
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
    aioSocket->_owner = this;

    // Add to the set of files
    _lock.lock();
    _sockets.addBack(aioSocket);
    _lock.unlock();

    return Error();
}

void AioServer::dropFile(AioFile* aioFile)
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

void AioServer::dropSocket(AioSocket* aioSocket)
{
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
}

bool AioServer::process()
{
    OVERLAPPED_EX* overlappedEx = NULL;
    DWORD bytesTransfered = 0;
    ULONG_PTR completionKey;
    uint32 completionValue;

    AioServer::fileCallback userFileCallback = NULL;
    AioServer::socketCallback userSocketCallback = NULL;
    AioServer::acceptCallback userAcceptCallback = NULL;
    AioServer::connectCallback userConnectCallback = NULL;

    WSABUF wsabuf;
    DWORD flags = 0;

    // Connect values
    sockaddr_in addressIpv4;
    sockaddr_in6 addressIpv6;
    sockaddr* addrPtr;
    int addrLen;

    Error error;
    BOOL bRet;
    int iRet;
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
                "AioServer::process");

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

        size_t socketsSize = _sockets.size();
        for (size_t i = 0; i < socketsSize; i++)
        {
            AioSocket* aioSocket = _sockets.get(i);
            bRet = ::CancelIo((HANDLE)aioSocket->_winSocket);

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
                            "AioServer::fileRead");
                    }
                }

                // Trigger callback if completed immediately or failed
                if (!completionQueued(winErr))
                {
                    userFileCallback = (AioServer::fileCallback)overlappedEx->callback;
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
                            "AioServer::fileWrite");
                    }
                }

                // Trigger callback if completed immediately or failed
                if (!completionQueued(winErr))
                {
                    userFileCallback = (AioServer::fileCallback)overlappedEx->callback;
                    userFileCallback(overlappedEx->aioFile,
                                     overlappedEx->userData,
                                     bytesTransfered,
                                     error);
                }

                break;
            case OP_ACCEPT:
                // Call AcceptEx
                bRet = acceptExPtr(overlappedEx->aioSocket->_winSocket, // Listen socket
                                   overlappedEx->acceptedSocket->_winSocket, // Accept socket
                                   overlappedEx->addressBuffer, // Output buffer (will hold addresses)
                                   0, // Receive data length
                                   ACCEPTEX_ADDRESS_SIZE, // Local address length
                                   ACCEPTEX_ADDRESS_SIZE, // Remote address length
                                   &bytesTransfered, // Bytes received
                                   &overlappedEx->overlapped); // Overlapped

                // Returns TRUE if completed immediately
                if (bRet == TRUE)
                {
                    // Skip this logic if not Vista+ as will get queued
#if (_WIN32_WINNT >= 0x0600)
                    error = handleAcceptSuccess(overlappedEx,
                                overlappedEx->aioSocket->_winSocket,
                                overlappedEx->acceptedSocket->_winSocket);
#endif
                }
                else
                {
                    winErr = ::WSAGetLastError();

                    if (winErr != ERROR_IO_PENDING)
                    {
                        error = WinUtil::getError(winErr,
                            "AcceptEx",
                            "AioServer::socketAccept");
                    }
                }

                // Trigger callback if completed immediately or failed
                if (!completionQueued(winErr))
                {
                    userAcceptCallback = (AioServer::acceptCallback)overlappedEx->callback;
                    userAcceptCallback(overlappedEx->aioSocket,
                                       overlappedEx->acceptedSocket,
                                       overlappedEx->userData,
                                       error);
                }
                break;

            case OP_CONNECT:
                // Fill out address information
                if (overlappedEx->address.getFamily() == INET_PROT_IPV4)
                {
                    ::memset(&addressIpv4, 0, sizeof(sockaddr_in));
                    addressIpv4.sin_family = AF_INET;
                    addressIpv4.sin_port = htons(overlappedEx->port);
                    ::memcpy(&addressIpv4.sin_addr, overlappedEx->address.getAddrData(), 4);

                    addrPtr = (sockaddr*)&addressIpv4;
                    addrLen = sizeof(addressIpv4);
                }
                else
                {
                    ::memset(&addressIpv6, 0, sizeof(sockaddr_in6));
                    addressIpv6.sin6_family = AF_INET6;
                    addressIpv4.sin_port = htons(overlappedEx->port);
                    ::memcpy(&addressIpv6.sin6_addr, overlappedEx->address.getAddrData(), 16);

                    addrPtr = (sockaddr*)&addressIpv6;
                    addrLen = sizeof(addressIpv6);
                }
                
                bRet = connectExPtr(overlappedEx->aioSocket->_winSocket, // Socket
                                    addrPtr, // Address
                                    addrLen, // Address length
                                    NULL, // Send buffer
                                    0, // Send buffer length
                                    NULL, // Bytes sent
                                    &overlappedEx->overlapped); // Overlapped

                // TRUE indicates immediate success, FALSE means failure or queued.
                if (bRet == TRUE)
                {
                    // Skip this logic if not Vista as will get queued
#if (_WIN32_WINNT >= 0x0600)
                    error = handleConnectSuccess(overlappedEx,
                                overlappedEx->aioSocket->_winSocket);
#endif
                }
                else
                {
                    winErr = ::WSAGetLastError();

                    if (winErr != ERROR_IO_PENDING)
                    {
                        error = WinUtil::getError(winErr,
                            "ConnectEx",
                            "AioServer::socketConnect");
                    }
                }

                // Trigger callback if completed immediately or failed
                if (!completionQueued(winErr))
                {
                    userConnectCallback = (AioServer::connectCallback)overlappedEx->callback;
                    userConnectCallback(overlappedEx->aioSocket,
                                        overlappedEx->userData,
                                        error);
                }

                break;
            case OP_DISCONNECT:
                bRet = disconnectExPtr(overlappedEx->aioSocket->_winSocket, // Socket
                                       &overlappedEx->overlapped, // Overlapped
                                       TF_REUSE_SOCKET, // Flags (not allowing reuse) // TEST
                                       0); // Reserved

                // TRUE indicates immediate success, FALSE means failure or queued.
                if (bRet != TRUE)
                {
                    winErr = ::GetLastError();

                    if (winErr != ERROR_IO_PENDING)
                    {
                        error = WinUtil::getError(winErr,
                            "DisconnectEx",
                            "AioServer::socketClose");
                    }
                }

                // Trigger callback if completed immediately or failed
                if (!completionQueued(winErr))
                {
                    userConnectCallback = (AioServer::connectCallback)overlappedEx->callback;
                    userConnectCallback(overlappedEx->aioSocket,
                                        overlappedEx->userData,
                                        error);
                }

                break;

            case OP_TRANSMIT_FILE:

                // Call TransmitFile
                bRet = transmitFilePtr(overlappedEx->aioSocket->_winSocket, // Socket handle
                                       overlappedEx->aioFile->_handle, // File handle
                                       overlappedEx->bufferSize, // Total bytes to send
                                       0, // Number of bytes per send (default)
                                       &overlappedEx->overlapped, // Pointer to OVERLAPPED
                                       NULL, // Extra buffers
                                       TF_USE_SYSTEM_THREAD); // flags

                // TRUE indicates immediate success, FALSE means failure or queued.
                if (bRet == TRUE)
                {
                    // If it succeeds, it appears we just assume it sent everything
                    bytesTransfered = overlappedEx->bufferSize;
                }
                else
                {
                    winErr = ::WSAGetLastError();

                    if (winErr != WSA_IO_PENDING)
                    {
                        // Trigger the callback
                        error = WinUtil::getError(winErr,
                            "TransmitFile",
                            "AioServer::socketSendFile");
                    }
                }

                // Trigger callback if completed immediately or failed
                if (!completionQueued(winErr))
                {
                    userFileCallback = (AioServer::fileCallback)overlappedEx->callback;
                    userFileCallback(overlappedEx->aioFile,
                                     overlappedEx->userData,
                                     overlappedEx->bufferSize,
                                     error);
                }

                break;
            case OP_RECV:

                wsabuf.buf = overlappedEx->buffer;
                wsabuf.len = overlappedEx->bufferSize;

                iRet = ::WSARecv(overlappedEx->aioSocket->_winSocket, // Socket handle
                                 &wsabuf, // Buffers
                                 1, // Buffer count
                                 &bytesTransfered, // Bytes recieved
                                 &flags, // Flags
                                 &overlappedEx->overlapped, // Pointer to OVERLAPPED
                                 NULL); // Completion routine

                // Returns 0 if completed immediately, SOCKET_ERROR on
                // failure or if queued.
                if (bRet != 0)
                {
                    winErr = ::WSAGetLastError();

                    // Oddly, it's possible to have no error set
                    if (winErr != ERROR_SUCCESS &&
                        winErr != WSA_IO_PENDING)
                    {
                        error = WinUtil::getError(winErr,
                            "WSARecv",
                            "AioServer::socketRead");
                    }
                }

                // Trigger callback if completed immediately or failed
                if (!completionQueued(winErr))
                {
                    userSocketCallback = (AioServer::socketCallback)overlappedEx->callback;
                    userSocketCallback(overlappedEx->aioSocket,
                                       overlappedEx->userData,
                                       bytesTransfered,
                                       error);
                }

                break;
            case OP_SEND:
                wsabuf.buf = overlappedEx->buffer;
                wsabuf.len = overlappedEx->bufferSize;

                iRet = ::WSASend(overlappedEx->aioSocket->_winSocket, // Socket handle
                                 &wsabuf, // Buffers
                                 1, // Buffer count
                                 &bytesTransfered, // Bytes sent
                                 0, // Flags
                                 &overlappedEx->overlapped, // Pointer to OVERLAPPED
                                 NULL); // Completion routine

                // Returns 0 if completed immediately, SOCKET_ERROR on
                // failure or if queued.
                if (iRet != 0)
                {
                    winErr = ::WSAGetLastError();

                    // Oddly, it's possible to have no error set
                    if (winErr != ERROR_SUCCESS &&
                        winErr != WSA_IO_PENDING)
                    {
                        error = WinUtil::getError(winErr,
                            "WSASend",
                            "AioServer::socketWrite");
                    }
                }

                // Trigger callback if completed immediately or failed
                if (!completionQueued(winErr))
                {
                    userSocketCallback = (AioServer::socketCallback)overlappedEx->callback;
                    userSocketCallback(overlappedEx->aioSocket,
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
                            "AioServer::fileRead");
                }

                userFileCallback = (AioServer::fileCallback)overlappedEx->callback;
                userFileCallback(overlappedEx->aioFile,
                                    overlappedEx->userData,
                                    0,
                                    error);

            case OP_WRITE_FILE:
                if (winErr != ERROR_SUCCESS)
                {
                    error = WinUtil::getError(winErr,
                            "WriteFile",
                            "AioServer::fileWrite");
                }

                userFileCallback = (AioServer::fileCallback)overlappedEx->callback;
                userFileCallback(overlappedEx->aioFile,
                                 overlappedEx->userData,
                                 bytesTransfered,
                                 error);
                break;
            case OP_ACCEPT:
                if (winErr == ERROR_SUCCESS)
                {
                    // Configure the socket and extract the connection info
                    error = handleAcceptSuccess(overlappedEx,
                                overlappedEx->aioSocket->_winSocket,
                                overlappedEx->acceptedSocket->_winSocket);
                }
                else
                {
                    error = WinUtil::getError(winErr,
                            "AcceptEx",
                            "AioServer::socketAccept");
                }

                userAcceptCallback = (AioServer::acceptCallback)overlappedEx->callback;
                userAcceptCallback(overlappedEx->aioSocket,
                                   overlappedEx->acceptedSocket,
                                   overlappedEx->userData,
                                   error);
                break;
            case OP_CONNECT:
                if (winErr == ERROR_SUCCESS)
                {
                    // A socket connected with ConnectEx is in default state until you
                    // force it to apply previous options with SO_UPDATE_CONNECT_CONTEXT
                    error = handleConnectSuccess(overlappedEx,
                                overlappedEx->aioSocket->_winSocket);
                }
                else
                {
                    error = WinUtil::getError(winErr,
                            "ConnectEx",
                            "AioServer::socketConnect");
                }

                userConnectCallback = (AioServer::connectCallback)overlappedEx->callback;
                userConnectCallback(overlappedEx->aioSocket,
                                    overlappedEx->userData,
                                    error);
                break;
            case OP_DISCONNECT:
                if (winErr != ERROR_SUCCESS)
                {
                    error = WinUtil::getError(winErr,
                            "DisconnectEx",
                            "AioServer::socketClose");
                }

                userConnectCallback = (AioServer::connectCallback)overlappedEx->callback;
                userConnectCallback(overlappedEx->aioSocket,
                                    overlappedEx->userData,
                                    error);
                break;
            case OP_TRANSMIT_FILE:
                if (winErr != ERROR_SUCCESS)
                {
                    error = WinUtil::getError(winErr,
                            "TransmitFile",
                            "AioServer::socketSendFile");
                }

                userSocketCallback = (AioServer::socketCallback)overlappedEx->callback;
                userSocketCallback(overlappedEx->aioSocket,
                                   overlappedEx->userData,
                                   bytesTransfered,
                                   error);
                break;

            case OP_RECV:
                if (winErr != ERROR_SUCCESS)
                {
                    error = WinUtil::getError(winErr,
                            "WSARecv",
                            "AioServer::socketRead");
                }

                userSocketCallback = (AioServer::socketCallback)overlappedEx->callback;
                userSocketCallback(overlappedEx->aioSocket,
                                   overlappedEx->userData,
                                   bytesTransfered,
                                   error);
                break;

            case OP_SEND:
                if (winErr != ERROR_SUCCESS)
                {
                    error = WinUtil::getError(winErr,
                            "WSASend",
                            "AioServer::socketWrite");
                }

                userSocketCallback = (AioServer::socketCallback)overlappedEx->callback;
                userSocketCallback(overlappedEx->aioSocket,
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

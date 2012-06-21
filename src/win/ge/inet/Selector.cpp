// Selector.cpp

#include <ge/inet/Selector.h>

#include <ge/inet/INetException.h>

#include <gepriv/WinUtil.h>

#include <cstdlib>
#include <Winsock2.h> // Must be before Windows.h


//#if (NTDDI_VERSION < NTDDI_VISTA)

/*
 * The Windows select function expects you to define FD_SETSIZE to an
 * acceptable maximum value for your selection set. It defaults to 64. In
 * reality, there is no maximum, it just wants a value for the Berkeley
 * style macros.
 *
 * Instead, just defining a custom struct with variable size and using
 * custom functions to set and clear the values. It's just an unsorted
 * array of SOCKET structs.
 */
#pragma warning(push)
#pragma warning(disable : 4200) // Nonstandard extension used
typedef struct
{
    u_int fd_count;
    SOCKET fd_array[];
} mod_fd_set;
#pragma warning(pop)


Selector::Selector()
{

}

Selector::~Selector()
{

}

void Selector::addSocket(Socket& socket, uint32 interestMask)
{
    m_socketList.addBack(&socket);
    m_interestList.addBack(interestMask);
}

bool Selector::removeSocket(Socket& socket)
{
    size_t index = -1;

    size_t socketCount = m_socketList.size();
    for (size_t i = 0; index < 0 && i < socketCount; i++)
    {
        if (socket.m_winSocket == m_socketList.get(i)->m_winSocket)
            index = i;
    }

    if (index == -1)
        return false;

    m_socketList.remove(index);
    m_interestList.remove(index);
    return true;
}

void Selector::select(List<SelectKey>* readyKeys)
{
    select(readyKeys, 0);
}

bool Selector::select(List<SelectKey>* readyKeys, uint32 timeout)
{
    size_t readCount = 0;
    size_t writeCount = 0;
    size_t errorCount = 0;

    size_t socketCount = m_socketList.size();

    // Count each type of status interest
    for (size_t i = 0; i < socketCount; i++)
    {
        uint32 interestMask = m_interestList.get(i);

        if (interestMask & INTEREST_READ)
        {
            readCount++;
        }

        if (interestMask & INTEREST_WRITE)
        {
            writeCount++;
        }

        if (interestMask & INTEREST_ERROR)
        {
            errorCount++;
        }
    }

    // Allocate FD sets
    mod_fd_set* readFdSet = (mod_fd_set*)
        ::malloc(sizeof(mod_fd_set) + (sizeof(SOCKET) * readCount));
    mod_fd_set* writeFdSet = (mod_fd_set*)
        ::malloc(sizeof(mod_fd_set) + (sizeof(SOCKET) * writeCount));
    mod_fd_set* errorFdSet = (mod_fd_set*)
        ::malloc(sizeof(mod_fd_set) + (sizeof(SOCKET) * errorCount));

    if (readFdSet == NULL ||
        writeFdSet == NULL ||
        errorFdSet == NULL)
    {
        if (readFdSet != NULL)
            ::free(readFdSet);
        if (writeFdSet != NULL)
            ::free(writeFdSet);
        throw std::bad_alloc();
    }

    readFdSet->fd_count = readCount;
    writeFdSet->fd_count = writeCount;
    errorFdSet->fd_count = errorCount;

    // Populate the FD sets
    size_t readIndex = 0;
    size_t writeIndex = 0;
    size_t errorIndex = 0;

    for (size_t i = 0; i < socketCount; i++)
    {
        SOCKET socket = m_socketList.get(i)->m_winSocket;
        uint32 interestMask = m_interestList.get(i);

        if (interestMask & INTEREST_READ)
        {
            readFdSet->fd_array[readIndex] = socket;
            readIndex++;
        }

        if (interestMask & INTEREST_WRITE)
        {
            writeFdSet->fd_array[writeIndex] = socket;
            writeIndex++;
        }

        if (interestMask & INTEREST_ERROR)
        {
            errorFdSet->fd_array[errorIndex] = socket;
            errorIndex++;
        }
    }

    // Do the select
    int res;
    
    if (timeout == 0)
    {
        res = ::select(0,
            (fd_set*)readFdSet,
            (fd_set*)writeFdSet,
            (fd_set*)errorFdSet,
            NULL);
    }
    else
    {
        timeval tv;
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;

        res = ::select(0,
            (fd_set*)readFdSet,
            (fd_set*)writeFdSet,
            (fd_set*)errorFdSet,
            &tv);  
    }

    DWORD err = 0;

    // Normal case, build result (other cases below)
    if (res > 0)
    {
        // Sort the result sets for faster lookups
        std::sort(readFdSet->fd_array, readFdSet->fd_array + readFdSet->fd_count);
        std::sort(writeFdSet->fd_array, writeFdSet->fd_array + writeFdSet->fd_count);
        std::sort(errorFdSet->fd_array, errorFdSet->fd_array + errorFdSet->fd_count);

        // For each socket, binary search to see if it's in the results
        for (size_t i = 0; i < socketCount; i++)
        {
            Socket* socket = m_socketList.get(i);
            SOCKET winSock = socket->m_winSocket;

            bool readSet = std::binary_search(readFdSet->fd_array,
                    readFdSet->fd_array + readFdSet->fd_count,
                    winSock);
            bool writeSet = std::binary_search(writeFdSet->fd_array,
                    writeFdSet->fd_array + writeFdSet->fd_count,
                    winSock);
            bool errorSet = std::binary_search(errorFdSet->fd_array,
                    errorFdSet->fd_array + errorFdSet->fd_count,
                    winSock);

            if (readSet || writeSet || errorSet)
            {
                SelectKey selectKey;
                selectKey.socket = socket;
                selectKey.interestMask = 0;

                if (readSet)
                    selectKey.interestMask |= INTEREST_READ;
                if (writeSet)
                    selectKey.interestMask |= INTEREST_WRITE;
                if (errorSet)
                    selectKey.interestMask |= INTEREST_ERROR;

                readyKeys->addBack(selectKey);
            }
        }
    }
    else
    {
        err = ::WSAGetLastError();
    }

    // Free no longer needed memory
    ::free(readFdSet);
    ::free(writeFdSet);
    ::free(errorFdSet);

    if (res == 0)
    {
        // Timeout
        return false;
    }
    else if (res == SOCKET_ERROR)
    {
        // Failure
        throw INetException(WinUtil::getErrorMessage(err));
    }

    return true;
}

// Old non-select version (limited to 64 fd)
/*
Selector::Selector()
{
    
}

Selector::~Selector()
{
    uint64 eventListSize = m_eventList.size();

    for (int i = 0; i < eventListSize; i++)
    {
        WSAEVENT wsaEvent = m_eventList.at(i);

        BOOL res = ::WSACloseEvent(wsaEvent);

        if (!res)
        {
            DWORD err = ::WSAGetLastError();

            // TODO: Log
        }
    }
}

void Selector::addSocket(const Socket& socket, uint32 eventMask)
{
    int interestMask = 0;

    if (m_socketList.size() >= 64)
    {
        throw INetException("Too many sockets added to Selector");
    }

    if (eventMask & SELECT_OP_ACCEPT)
    {
        interestMask |= FD_ACCEPT;
    }
    
    if (eventMask & SELECT_OP_CONNECT)
    {
        interestMask |= FD_CONNECT;
    }
    
    if (eventMask & SELECT_OP_READ)
    {
        interestMask |= FD_READ;
    }
    
    if (eventMask & SELECT_OP_WRITE)
    {
        interestMask |= FD_WRITE;
    }

    // Create the WSAEVENT
    WSAEVENT wsaEvent = ::WSACreateEvent();

    // Should only fail on catastrophic error
    if (wsaEvent == WSA_INVALID_EVENT)
    {
        throw INetException(WinUtil::getWSALastErrorMessage());
    }

    // Add the SOCKET to our WSAEVENT
    int ret = ::WSAEventSelect(socket.m_socket, wsaEvent, interestMask);

    if (ret)
    {
        throw INetException(WinUtil::getWSALastErrorMessage());
    }

    m_socketList.addBack(&socket);
    m_eventList.addBack(wsaEvent);
}

bool Selector::removeSocket(const Socket& socket)
{
    int dataSize = m_socketList.size();

    for (int i = 0; i < dataSize; i++)
    {
        const Socket* socketPtr = m_socketList.get(i);

        if (socketPtr != &socket)
            continue;

        WSAEVENT wsaEvent = m_eventList.get(i);

        // Remove the socket from our WSAEVENT
        int ret = ::WSAEventSelect(socket.m_socket, wsaEvent, 0);

        if (ret)
        {
            // Save the error
            DWORD err = ::WSAGetLastError();

            // Try to close the event (ignoring any failure)
            ::WSACloseEvent(wsaEvent);

            // Throw the origional failure
            throw INetException(WinUtil::getErrorMessage(err));
        }

        // Close the WSAEVENT
        BOOL res = ::WSACloseEvent(wsaEvent);

        if (!res)
        {
            throw INetException(WinUtil::getWSALastErrorMessage());
        }

        return true;
    }

    return false;
}

void Selector::select(List<SelectKey>* readyKeys)
{
    void select(readyKeys, 0);
}

bool Selector::select(List<SelectKey>* readyKeys, uint32 timeout)
{
    DWORD dataSize = m_socketList.size();

    if (dataSize == 0)
        return false;

    if (timeout <= 0)
        timeout = WSA_INFINITE;

    DWORD waitRet = ::WSAWaitForMultipleEvents(dataSize,
            &m_eventList[0],
            FALSE,
            timeout,
            FALSE);

    if (waitRet == WSA_WAIT_TIMEOUT)
    {
        return false;
    }
    else if (waitRet == WSA_WAIT_FAILED)
    {
        throw INetException(WinUtil::getWSALastErrorMessage());
    }

    DWORD index = waitRet - WSA_WAIT_EVENT_0;
    WSANETWORKEVENTS networkEvents;
    ::memset(&networkEvents, 0, sizeof(networkEvents));

    while (index < dataSize)
    {
        waitRet = ::WSAWaitForMultipleEvents(dataSize - index,
                m_eventList.data() + index,
                FALSE,
                0,
                FALSE);

        if ((waitRet != WSA_WAIT_FAILED) &&
            (waitRet != WSA_WAIT_TIMEOUT))
        {
            int enumRet = ::WSAEnumNetworkEvents(m_socketList.get(index)->m_socket,
                    m_eventList.data() + index,
                    &networkEvents);

            if (enumRet == SOCKET_ERROR)
            {
                throw INetException(WinUtil::getWSALastErrorMessage());
            }
        }

        index = waitRet - WSA_WAIT_EVENT_0;
    }

    return true;
}
*/

//#else // Vista or later

/*

Selector::Selector()
{

}

Selector::~Selector()
{

}

void Selector::addSocket(const Socket& socket, uint32 interestMask)
{

}

bool Selector::removeSocket(const Socket& socket)
{

}

void Selector::select(List<SelectKey>* readyKeys)
{
    select(readyKeys, 0);
}

bool Selector::select(List<SelectKey>* readyKeys, uint32 timeout)
{

}

#endif // #if (NTDDI_VERSION < NTDDI_VISTA)

*/
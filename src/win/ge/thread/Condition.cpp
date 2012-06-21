// Condition.cpp

#include <ge/thread/Condition.h>

/*
 * Windows implementation of a condition for multithreaded signaling. This
 * class is somewhat complex to allow signalAll() to work correctly. Windows
 * does not have an equivalent function available except in Vista+.
 *
 * If you are willing to be Vista only this class can be replaced with simple
 * usage of a CONDITION_VARIABLE and CRITICAL_SECTION.
 *
 * Implementation based on the article:
 * "Strategies for Implementing UNIX Condition Variables on Win32"
 * http://www.cs.wustl.edu/~schmidt/win32-cv-1.html.
 */

#if (NTDDI_VERSION >= NTDDI_VISTA)

Condition::Condition()
{
    ::InitializeCriticalSection(&m_conditionLock);
    ::InitializeConditionVariable(&m_conditionVariable);
}

Condition::~Condition()
{
    ::DeleteCriticalSection(&m_conditionLock);
}

void Condition::lock()
{
    ::EnterCriticalSection(&m_conditionLock);
}

void Condition::unlock()
{
    ::LeaveCriticalSection(&m_conditionLock);
}

void Condition::signal()
{
    ::WakeConditionVariable(&m_conditionVariable);
}

void Condition::signalAll()
{
    ::WakeAllConditionVariable(&m_conditionVariable);
}

void Condition::wait()
{
    waitImpl(INFINITE);
}

uint32 Condition::wait(uint32 milliseconds)
{
    // Note that timeGetTime is more accurate, but appears more expensive
    // and requires linking another library.
    uint32 startTime = ::GetTickCount();

    waitImpl(milliseconds);

    uint32 endTime = ::GetTickCount();
    return (endTime - startTime);
}

void Condition::waitImpl(uint32 milliseconds)
{
    ::SleepConditionVariableCS(&m_conditionVariable, &m_conditionLock, milliseconds);
}

#else

Condition::Condition()
{
    m_waitingThreads = 0;
    m_waitGenerationCount = 0;
    m_threadsToRelease = 0;

    // Create a manual-reset event.
    m_event = ::CreateEventW(NULL,  // Default security
                             TRUE,  // Manual-reset event
                             FALSE, // Start non-signaled
                             NULL); // Unnamed event
}

Condition::~Condition()
{
    ::DeleteCriticalSection(&m_dataLock);
    ::CloseHandle(m_event);
}

void Condition::lock()
{
    ::EnterCriticalSection(&m_conditionLock);
}

void Condition::unlock()
{
    ::LeaveCriticalSection(&m_conditionLock);
}

void Condition::signal()
{
    if (m_waitingThreads > m_threadsToRelease)
    {
        // Signal the manual-reset event.
        ::SetEvent(m_event);

        // Increment the number of threads to release by 1
        m_threadsToRelease++;

        // Start a new generation.
        m_waitGenerationCount++;
    }
}

void Condition::signalAll()
{
    if (m_waitingThreads > 0)
    {
        // Signal the manual-reset event.
        ::SetEvent(m_event);

        // Release all the threads in this generation.
        m_threadsToRelease = m_waitingThreads;

        // Start a new generation.
        m_waitGenerationCount++;
    }
}

void Condition::wait()
{
    waitImpl(INFINITE);
}

uint32 Condition::wait(uint32 milliseconds)
{
    // Note that timeGetTime is more accurate, but appears more expensive
    // and requires linking another library.
    uint32 startTime = ::GetTickCount();

    waitImpl(milliseconds);

    uint32 endTime = ::GetTickCount();
    return (endTime - startTime);
}

void Condition::waitImpl(uint32 milliseconds)
{
    // Increment count of waiters.
    m_waitingThreads++;

    // Store the current generation number.
    int myGeneration = m_waitGenerationCount;

    // Release the critical section
    ::LeaveCriticalSection(&m_conditionLock);

    while (true)
    {
        // Wait until the event is signaled.
        DWORD waitResult = ::WaitForSingleObject(m_event, milliseconds);

        ::EnterCriticalSection(&m_conditionLock);

        // Break if there are still waiting threads from this generation
        // that haven't been released from this wait yet, or if the wait
        // times out.
        bool waitDone = ((m_threadsToRelease > 0 &&
                          m_waitGenerationCount != myGeneration) ||
                          (waitResult == WAIT_TIMEOUT));

        if (waitDone)
        {
            // Keep acquired
            break;
        }
        else
        {
            ::LeaveCriticalSection(&m_conditionLock);
        }
    }

    // Update the information about the waiting threads
    m_waitingThreads--;
    m_threadsToRelease--;
    int last_waiter = m_threadsToRelease == 0;

    if (last_waiter)
    {
        // We're the last waiter to be notified, so reset the manual event.
        ::ResetEvent(m_event);
    }

    // Re-acquire the critical section
    ::EnterCriticalSection(&m_conditionLock);
}

#endif

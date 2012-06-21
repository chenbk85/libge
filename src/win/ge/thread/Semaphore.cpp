// Semaphore.cpp

#include "ge/thread/Semaphore.h"

Semaphore::Semaphore() :
    _sema(INVALID_HANDLE_VALUE)
{
}

Semaphore::~Semaphore()
{
    ::CloseHandle(_sema);
}

Semaphore::Semaphore(Semaphore&& other)
{
    _sema = other._sema;
}

Semaphore& Semaphore::operator=(Semaphore&& other)
{
    _sema = other._sema;
    return *this;
}

void Semaphore::init()
{
    init(0);
}

void Semaphore::init(uint32 initialCount)
{
    _sema = ::CreateSemaphoreW(NULL,
                               initialCount,
                               4294967295U, // 2^32-1
                               NULL);
}

void Semaphore::increment()
{
    ::ReleaseSemaphore(_sema, 1, NULL);
}

void Semaphore::decrement()
{
    ::WaitForSingleObject(_sema, INFINITE);
}

uint32 Semaphore::decrement(uint32 millis)
{
    uint32 startTime = ::GetTickCount();

    ::WaitForSingleObject(_sema, millis);

    uint32 endTime = ::GetTickCount();

    return (endTime - startTime);
}

bool Semaphore::tryDecrement()
{
    DWORD res = ::WaitForSingleObject(_sema, 0);

    if (res == WAIT_OBJECT_0)
    {
        return true;
    }
    else if (res == WAIT_ABANDONED)
    {
        return false;
    }
    else
    {
        // TODO: Handle
    }

    return false;
}

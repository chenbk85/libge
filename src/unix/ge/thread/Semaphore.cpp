// Semaphore.cpp

#include "ge/thread/Semaphore.h"

#include <time.h>
#include <errno.h>

Semaphore::Semaphore() :
    _sema(0)
{
}

Semaphore::~Semaphore()
{
    ::sem_destroy(&_sema);
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
    // Check that initialCount < INT_MAX

    int res = ::sem_init(&_sema, 0, initialCount);

    if (res != 0)
    {
        // TODO: throw?
    }
}

void Semaphore::increment()
{
    int res = ::sem_post(&_sema);

    if (res != 0)
    {
        // TODO: Overflowed, throw?
    }
}

void Semaphore::decrement()
{
    int res = -1;
    
    do
    {
        res = ::sem_wait(&_sema);
    } while (res == -1 && errno == EINTR);

    if (res != 0)
    {
        // TODO: Handle?
    }
}

uint32 Semaphore::decrement(uint32 millis)
{
    timespec curspec;
    timespec waitspec;
    timespec endspec;

    // Get the current time
    clock_gettime(CLOCK_REALTIME, &curspec);

    // Add in the wait time
    waitspec.tv_sec = curspec.tv_sec + (millis / 1000);
    waitspec.tv_nsec = curspec.tv_nsec + ((millis % 1000) * 1000000);

    if (tspec.tv_nsec > 999999999)
    {
        tspec.tv_sec++;
        tspec.tv_nsec -= 1000000000;
    }

    int res = -1;
    
    do
    {
        res = ::sem_timedwait(&_sema, &waitspec);
    } while (res == -1 && errno == EINTR);

    if (res != 0)
    {
        int err = errno;

        if (err != ETIMEDOUT)
        {
            // TODO: Handle?
        }
    }

    // Get the time after the wait
    clock_gettime(CLOCK_REALTIME, &endspec);

    uint32 res = endspec.tv_sec - curspec.tv_sec;
    res += (endspec.tv_nsec - curspec.tv_nsec) / 1000000;

    return res;
}

bool Semaphore::tryDecrement()
{
    int res = -1;
    
    do
    {
        res = ::sem_trywait(&_sema);
    } while (res == -1 && errno == EINTR);

    if (res == 0)
    {
        return true;
    }
    else
    {
        int err = errno;

        if (err == EAGAIN)
        {
            return false;
        }
        else
        {
            // TODO: Handle?
        }
    }
}
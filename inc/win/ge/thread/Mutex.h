// Mutex.h

#ifndef MUTEX_H
#define MUTEX_H

#include <ge/common.h>

#define _WINSOCKAPI_
#include <Windows.h>

/*
 * Windows mutex implementation. Wrapper around a CRITICAL_SECTION.
 */
class Mutex
{
public:
    Mutex();
    ~Mutex();

    void lock();
    bool trylock();
    void unlock();

private:
    Mutex(const Mutex& other);
    Mutex& operator=(const Mutex& other);

private:
    CRITICAL_SECTION m_criticalSection;
};

#endif // MUTEX_H

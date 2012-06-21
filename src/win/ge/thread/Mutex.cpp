// Mutex.cpp

#include <ge/thread/Mutex.h>

Mutex::Mutex()
{
    ::InitializeCriticalSection(&m_criticalSection);
}

Mutex::~Mutex()
{
    ::DeleteCriticalSection(&m_criticalSection);
}

void Mutex::lock()
{
    ::EnterCriticalSection(&m_criticalSection);
}

bool Mutex::trylock()
{
    return ::TryEnterCriticalSection(&m_criticalSection) != 0;
}

void Mutex::unlock()
{
    ::LeaveCriticalSection(&m_criticalSection);
}

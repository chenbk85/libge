// Semaphore.h

#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <ge/common.h>

#define _WINSOCKAPI_
#include <Windows.h>

/*
 * Object wrapping a semaphore that blocks if you attempt to decriment it to
 * zero.
 */
class Semaphore
{
public:
    Semaphore();
    ~Semaphore();

    Semaphore(Semaphore&& other);
    Semaphore& operator=(Semaphore&& other);

    void init();
    void init(uint32 initialCount);

    void increment();
    void decrement();
    uint32 decrement(uint32 millis);
    bool tryDecrement();

private:
    Semaphore(const Semaphore& other) DELETED;
    Semaphore& operator=(const Semaphore& other) DELETED;

    HANDLE _sema;
};

#endif // SEMAPHORE_H

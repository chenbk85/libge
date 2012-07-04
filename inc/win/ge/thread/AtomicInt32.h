// AtomicInt32.h

#ifndef ATOMIC_INT32_H
#define ATOMIC_INT32_H

#include <ge/common.h>

#include <Windows.h>

/*
 * Object wrapping an atomic int32
 */
class AtomicInt32
{
public:
    AtomicInt32() :
        _value(0)
    {}

    explicit AtomicInt32(int32 val) :
        _value(val)
    {}

    int32 inc()
    {
        return ::InterlockedIncrement(&_value);
    }

    int32 dec()
    {
        return ::InterlockedDecrement(&_value);
    }

    int32 add(int32 val)
    {
        return ::InterlockedAdd(&_value, val);
    }

    int32 sub(int32 val)
    {
        val *= -1; // There is no InterlockedSubtract
        return ::InterlockedAdd(&_value, val);
    }

    int32 bitwiseOr(int32 val)
    {
        return ::InterlockedOr(&_value, val);
    }

    int32 bitwiseAnd(int32 val)
    {
        return ::InterlockedAnd(&_value, val);
    }

    int32 bitwiseXor(int32 val)
    {
        return ::InterlockedXor(&_value, val);
    }

    bool compareAndExchange(int32 oldVal, int32 newVal)
    {
        return (::InterlockedCompareExchange(&_value, newVal, oldVal) == oldVal);
    }

private:
    AtomicInt32(const AtomicInt32& other) DELETED;
    AtomicInt32& operator=(const AtomicInt32& other) DELETED;

    volatile LONG _value;
};


#endif // ATOMIC_INT32_H

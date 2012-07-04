// AtomicInt32.h

#ifndef ATOMIC_INT32_H
#define ATOMIC_INT32_H

#include <ge/common.h>

// TODO: Needs pthread backup

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
        return __sync_add_and_fetch(&_value, 1);
    }

    int32 dec()
    {
        return __sync_sub_and_fetch(&_value, 1);
    }

    int32 add(int32 val)
    {
        return __sync_add_and_fetch(&_value, val);
    }

    int32 sub(int32 val)
    {
        return __sync_sub_and_fetch(&_value, val);
    }

    int32 bitwiseOr(int32 val)
    {
        return __sync_or_and_fetch(&_value, val);
    }

    int32 bitwiseAnd(int32 val)
    {
        return __sync_and_and_fetch(&_value, val);
    }

    int32 bitwiseXor(int32 val)
    {
        return __sync_xor_and_fetch(&_value, val);
    }

    bool compareAndExchange(int32 oldVal, int32 newVal)
    {
        return __sync_bool_compare_and_swap(&_value, oldVal, newVal);
    }

private:
    AtomicInt32(const AtomicInt32& other) DELETED;
    AtomicInt32& operator=(const AtomicInt32& other) DELETED;

    long _value;
};


#endif // ATOMIC_INT32_H

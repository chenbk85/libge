// Hasher.h

#ifndef HASHER_H
#define HASHER_H

#include <ge/common.h>

// Primary class template. Will try to call a function called hash on the
// type.
template<typename T>
struct hasher
{
    uint32 operator()(T val) const
    {
        return val.hash();
    }
};

// Partial specialization for pointer types.
template<typename T>
struct hasher<T*>
{
    uint32 operator()(T* val) const
    {
        return (uint32)(val & 0xFFFFFFFF);
    }
};

// Explicit specializations for integer types.
template<>
inline uint32 hasher<int8>::operator()(int8 val) const
{
    return (uint32)val;
}

template<>
inline uint32 hasher<int16>::operator()(int16 val) const
{
    return (uint32)val;
}

template<>
inline uint32 hasher<int32>::operator()(int32 val) const
{
    return (uint32)val;
}

template<>
inline uint32 hasher<const int32&>::operator()(const int32& val) const
{
    return (uint32)val;
}

template<>
inline uint32 hasher<int64>::operator()(int64 val) const
{
    uint32 high = (uint32)(val >> 32);
    uint32 low = (uint32)(val & 0xFFFFFFFF);
    return (uint32)(high & low);
}

template<>
inline uint32 hasher<uint8>::operator()(uint8 val) const
{
    return (uint32)val;
}

template<>
inline uint32 hasher<uint16>::operator()(uint16 val) const
{
    return (uint32)val;
}

template<>
inline uint32 hasher<uint32>::operator()(uint32 val) const
{
    return (uint32)val;
}

template<>
inline uint32 hasher<uint64>::operator()(uint64 val) const
{
    uint32 high = (uint32)(val >> 32);
    uint32 low = (uint32)(val & 0xFFFFFFFF);
    return (uint32)(high & low);
}

#endif // HASHER_H

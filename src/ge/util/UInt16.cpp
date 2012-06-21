// UInt16.cpp

#include "ge/util/UInt16.h"

#include "ge/util/Bool.h"
#include "ge/util/UInt32.h"
#include "ge/util/UtilData.h"

#include <cassert>

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_rotl16, _rotr16)
#endif

String UInt16::uint16ToString(uint16 value, uint32 radix)
{
    return UInt32::uint32ToString((uint32)value, radix);
}

uint32 UInt16::uint16ToBuffer(char* buffer, uint32 bufferLen, uint16 value, uint32 radix)
{
    return UInt32::uint32ToBuffer(buffer, (uint32)value, radix);
}

uint16 UInt16::parseUInt16(StringRef strRef, bool* ok, uint32 radix)
{
    uint32 ret = UInt32::parseUInt32(strRef, ok, radix);

    if (ret > UINT16_MAX)
    {
        Bool::setBool(ok, false);
        return 0;
    }

    return (uint16)ret;
}

uint32 uint16CountOnes(uint16 value)
{
    uint8 high = value >> 8;
    uint8 low = value & 0xFF;
    return UtilData::g_onesCountsTable[high] +
           UtilData::g_onesCountsTable[low];
}

uint32 uint16CountZeros(uint16 value)
{
    return 16 - uint16CountOnes(value);
}

bool uint16Parity(uint16 value)
{
    return uint16CountOnes(value) & 1;
}

bool uint16IsPow2(uint16 value)
{
    return value && !(value & (value - 1));;
}

uint16 uint16NextPow2(uint16 value)
{
    if (value == 0)
        return 1;

    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value++;
    return value;
}

uint16 uint16RotateLeft(uint16 value, uint8 shift)
{
    assert(shift >= 0 && shift <= 16);

#if defined(_MSC_VER)
    return _rotl16(value, shift);
#else
    return (value << shift) | (value >> (16 - shift));
#endif
}

uint16 uint16RotateRight(uint16 value, uint8 shift)
{
    assert(shift >= 0 && shift <= 16);

#if defined(_MSC_VER)
    return _rotr16(value, shift);
#else
    return (value >> shift) | (value << (16 - shift));
#endif
}

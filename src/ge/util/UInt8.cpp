// UInt8.cpp

#include <ge/util/UInt8.h>

#include <ge/util/Bool.h>
#include <ge/util/UInt32.h>
#include <ge/util/UtilData.h>

#include <cassert>

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_rotl8, _rotr8)
#endif

// Table of if the given byte is a power of 2.
static bool uint8_isPow2Table[] = {
    0, 1, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, // 00-0F
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 10-1F
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 20-2F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 30-3F
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 40-4F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 50-5F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 60-6F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 70-7F
    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 80-8F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 90-9F
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // A0-AF
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // B0-BF
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // C0-CF
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // D0-DF
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // E0-EF
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  // F0-FF
};

String UInt8::uint8ToString(uint8 value, uint32 radix)
{
    return UInt32::uint32ToString((uint32)value, radix);
}

uint32 UInt8::uint8ToBuffer(char* buffer, uint32 bufferLen, uint8 value, uint32 radix)
{
    return UInt32::uint32ToBuffer(buffer, (uint32)value, radix);
}

uint8 UInt8::parseUInt8(StringRef strRef, bool* ok, uint32 radix)
{
    uint32 ret = UInt32::parseUInt32(strRef, ok, radix);

    if (ret > UINT8_MAX)
    {
        Bool::setBool(ok, false);
        return 0;
    }

    return (uint8)ret;
}

uint32 UInt8::uint8CountOnes(uint8 value)
{
    return UtilData::g_onesCountsTable[value];
}

uint32 UInt8::uint8CountZeros(uint8 value)
{
    return 8 - UtilData::g_onesCountsTable[value];
}

bool UInt8::uint8Parity(uint8 value)
{
    return UtilData::g_onesCountsTable[value] & 1;
}

bool UInt8::uint8IsPow2(uint8 value)
{
    return uint8_isPow2Table[value];
}

uint8 UInt8::uint8NextPow2(uint8 value)
{
    if (value == 0)
        return 1;

    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value++;
    return value;
}

uint8 UInt8::uint8RotateLeft(uint8 value, uint8 shift)
{
    assert(shift >= 0 && shift <= 8);

#if defined(_MSC_VER)
    return _rotl8(value, shift);
#else
    return (value << shift) | (value >> (8 - shift));
#endif
}

uint8 UInt8::uint8RotateRight(uint8 value, uint8 shift)
{
    assert(shift >= 0 && shift <= 8);

#if defined(_MSC_VER)
    return _rotr8(value, shift);
#else
    return (value >> shift) | (value << (8 - shift));
#endif
}
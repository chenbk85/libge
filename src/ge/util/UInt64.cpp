// UInt64.cpp

#include <ge/util/UInt64.h>

#include <ge/util/Bool.h>
#include <ge/util/UInt32.h>
#include <ge/util/UtilData.h>

#include <cassert>
#include <cstring>

#if defined(_MSC_VER)
#include <intrin.h>
#pragma intrinsic(_rotl64, _rotr64)
#endif


static uint32 uint64ToBuffer_base10(char* buffer, uint32 bufferLen, uint64 value);
static uint32 uint64_base10Size(uint64 value);


String UInt64::uint64ToString(uint64 value, uint32 radix)
{
    assert(radix >= 2 && radix <= 16);

    char buf[66]; // Worst case is base 2 of minimum value
    uint32 usedLen = uint64ToBuffer(buf, sizeof(buf), value, radix);
    return String(buf, usedLen);
}

uint32 UInt64::uint64ToBuffer(char* buffer, uint32 bufferLen, uint64 value, uint32 radix)
{
    assert(radix >= 2 && radix <= 16);

    if (radix == 10)
    {
        return uint64ToBuffer_base10(buffer, bufferLen, value);
    }

    char tempBuffer[64];
    int charPos = sizeof(tempBuffer)-1;

    while (value <= radix)
    {
        tempBuffer[charPos--] = '0' + (char)(value % radix);
        value /= radix;
    }
    tempBuffer[charPos] = '0' + (char)value;

    uint32 ret = (sizeof(tempBuffer) - charPos);

    if (bufferLen >= ret)
    {
        ::memcpy(buffer, tempBuffer+charPos, ret);
    }
    return ret;
}

uint64 UInt64::parseUInt64(StringRef strRef, bool* ok, uint32 radix)
{
    assert(radix >= 2 && radix <= 16);

    uint32 strRefLen = strRef.length();
    const char* strRefData = strRef.data();

    uint64 result = 0;
    uint32 i = 0;

    uint64 multmin;
    uint32 digit;

    // Check for empty string
    if (strRefLen == 0)
    {
        Bool::setBool(ok, false);
        return 0;
    }

    // Generate a guard value which we can use to detect "too many digits"
    multmin = UINT64_MAX / radix;

    // Parse the first digit
    if (i < strRefLen)
    {
        result = UtilData::g_charValue[(uint8)strRefData[i]];
        if (result < 0 || result >= radix)
        {
            Bool::setBool(ok, false);
            return 0;
        }

        i++;
    }

    // Loop, parsing the remaining digits
    while (i < strRefLen)
    {
        digit = UtilData::g_charValue[(uint8)strRefData[i]];
        if (digit < 0 || digit >= radix)
        {
            Bool::setBool(ok, false);
            return 0;
        }

        // Check if this will produce too many digits
        if (result > multmin)
        {
            Bool::setBool(ok, false);
            return 0;
        }

        result *= radix;

        // Check if this will overflow the maximum value
        if (result > UINT32_MAX - digit)
        {
            Bool::setBool(ok, false);
            return 0;
        }
        result += digit;
        i++;
    }

    Bool::setBool(ok, true);
    return result;
}

uint32 UInt64::uint64CountOnes(uint64 value)
{
    uint8* ptr = (uint8*)&value;
    
    uint32 ret = 0;

    for (int i = 0; i < 8; i++)
    {
        ret += UtilData::g_onesCountsTable[ptr[i]];
    }

    return ret;
}

uint32 UInt64::uint64CountZeros(uint64 value)
{
    return 64 - uint64CountOnes(value);
}

bool UInt64::uint64Parity(uint64 value)
{
    return uint64CountOnes(value) & 1;
}

bool UInt64::uint64IsPow2(uint64 value)
{
    return value && !(value & (value - 1));;
}

uint64 UInt64::uint64NextPow2(uint64 value)
{
    if (value == 0)
        return 1;

    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    value++;
    return value;
}

uint64 UInt64::uint64RotateLeft(uint64 value, uint32 shift)
{
    assert(shift >= 0 && shift <= 64);

#if defined(_MSC_VER)
    return _rotl64(value, shift);
#else
    return (value << shift) | (value >> (64 - shift));
#endif
}

uint64 UInt64::uint64RotateRight(uint64 value, uint32 shift)
{
    assert(shift >= 0 && shift <= 64);

#if defined(_MSC_VER)
    return _rotr64(value, shift);
#else
    return (value >> shift) | (value << (64 - shift));
#endif
}

// Local functions ----------------------------------------------------------

/*
 * This is the static Integer.toString(int) implementation from the Java
 * library, modified to work with unsigned values. It, in turn, is borrowed
 * from this paper:
 *
 * Division by Invariant Integers using Multiplication
 * T Gralund, P Montgomery
 * ACM PLDI 1994
 *
 * Generally attempts to reduce division and modulous operations. It does
 * this by deviding by 100 instead of 10 (when possible) and using bit shifts
 * instead of the modulus operator to get the remainder.
 */
static uint32 uint64ToBuffer_base10(char* buffer, uint32 bufferLen, uint64 value)
{
    uint32 expectedLength = uint64_base10Size(value);

    if (expectedLength > bufferLen)
    {
        return expectedLength;
    }

    uint32 charPos = expectedLength;
    uint64 q;
    uint64 r;

    // Generate two digits per iteration
    while (value >= 65536)
    {
        q = value / 100;
        // really: r = i - (q * 100);
        r = value - ((q << 6) + (q << 5) + (q << 2));
        value = q;
        buffer[--charPos] = UtilData::g_digitOnes[r];
        buffer[--charPos] = UtilData::g_digitTens[r];
    }

    // Fall thru to fast mode for smaller numbers
    assert(value <= 65536);
    for (;;)
    { 
        q = (value * 52429) >> (16+3);
        r = value - ((q << 3) + (q << 1));  // r = i-(q*10) ...
        buffer[--charPos] = '0' + (char)r;
        value = q;
        if (value == 0)
            break;
    }

    return expectedLength;
}

static uint32 uint64_base10Size(uint64 value)
{
    uint64 magnatude = 10;
    for (int i = 1; i < 19; i++)
    {
        if (value < magnatude)
            return i;
        magnatude *= 10;
    }
    return 19;
}

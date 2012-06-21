// UInt32.cpp

#include "ge/util/UInt32.h"

#include "ge/util/Bool.h"
#include "ge/util/UtilData.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

// Table used for avoiding a tiny bit of math when estimating the length of
// a uint32
static const uint32 uint32_base10SizeTable[] = {
    9,
    99,
    999,
    9999,
    99999,
    999999,
    9999999,
    99999999,
    999999999,
    2147483647
};

static uint32 uint32ToBuffer_base10(char* buffer, uint32 bufferLen, uint32 value);
static uint32 uint32_base10Size(uint32 value);


String UInt32::uint32ToString(uint32 value, uint32 radix)
{
    assert(radix >= 2 && radix <= 16);

    char buf[34]; // Worst case is base 2 of minimum value
    uint32 usedLen = uint32ToBuffer(buf, value, sizeof(buf), radix);
    return String(buf, usedLen);
}

uint32 UInt32::uint32ToBuffer(char* buffer, uint32 bufferLen, uint32 value, uint32 radix)
{
    assert(radix >= 2 && radix <= 16);

    if (radix == 10)
    {
        return uint32ToBuffer_base10(buffer, bufferLen, value);
    }

    char tempBuffer[33];
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

uint32 UInt32::parseUInt32(StringRef strRef, bool* ok, uint32 radix)
{
    assert(radix >= 2 && radix <= 16);

    uint32 strRefLen = strRef.length();
    const char* strRefData = strRef.data();

    uint32 result = 0;
    uint32 i = 0;

    uint32 multmin;
    uint32 digit;

    // Check for empty string
    if (strRefLen == 0)
    {
        Bool::setBool(ok, false);
        return 0;
    }

    // Generate a guard value which we can use to detect "too many digits"
    multmin = UINT32_MAX / radix;

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

uint32 UInt32::uint32CountOnes(uint32 value)
{
    uint8* ptr = (uint8*)&value;
    
    uint32 ret = 0;

    for (int i = 0; i < 4; i++)
    {
        ret += UtilData::g_onesCountsTable[ptr[i]];
    }

    return ret;
}

uint32 UInt32::uint32CountZeros(uint32 value)
{
    return 32 - uint32CountOnes(value);
}

/*
 * Implementation from here:
 * http://graphics.stanford.edu/~seander/bithacks.html
 */
bool UInt32::uint32Parity(uint32 value)
{
    return uint32CountOnes(value) & 1;
}

/*
 * Implementation from here:
 * http://graphics.stanford.edu/~seander/bithacks.html
 */
bool UInt32::uint32IsPow2(uint32 value)
{
    return value && !(value & (value - 1));
}

/*
 * Implementation from here:
 * http://graphics.stanford.edu/~seander/bithacks.html
 */
uint32 UInt32::uint32NextPow2(uint32 value)
{
    if (value == 0)
        return 1;

    value--;
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value++;
    return value;
}

uint32 uint32RotateLeft(uint32 value, uint32 shift)
{
    assert(shift >= 0 && shift <= 32);

#if defined(_MSC_VER)
    return _rotl(value, shift);
#else
    return (value << shift) | (value >> (32 - shift));
#endif
}

uint32 uint32RotateRight(uint32 value, uint32 shift)
{
    assert(shift >= 0 && shift <= 32);

#if defined(_MSC_VER)
    return _rotr(value, shift);
#else
    return (value >> shift) | (value << (32 - shift));
#endif
}

// Local Functions ----------------------------------------------------------

/*
 * This is the static Integer.toString(int) implementation from the Java
 * library, modified to work with unsigned values. It, in turn, is borrowed
 * from this paper:
 *
 * Division by Invariant Integers using Multiplication
 * T Gralund, P Montgomery
 * ACM PLDI 1994
 *
 * Generally attempts to eliminate division and modulous operations. It does
 * this by deviding by 100 instead of 10 (when possible) and using bit shifts
 * instead of the modulus operator to get the remainder.
 */
static uint32 uint32ToBuffer_base10(char* buffer, uint32 bufferLen, uint32 value)
{
    uint32 expectedLength = uint32_base10Size(value);

    if (expectedLength > bufferLen)
    {
        return expectedLength;
    }

    uint32 charPos = expectedLength;
    uint32 q;
    uint32 r;

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
        buffer[--charPos] = '0' + r;
        value = q;
        if (value == 0)
            break;
    }

    return expectedLength;
}

/*
 * Determines the size of the passed uint32 as a base 10 string, not
 * including a nul character.
 */
static uint32 uint32_base10Size(uint32 value)
{
    for (int i = 0; ;i++)
    {
        if (value <= uint32_base10SizeTable[i])
            return i+1;
    }
}

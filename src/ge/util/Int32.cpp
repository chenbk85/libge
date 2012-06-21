// Int32.cpp

#include "ge/util/Int32.h"

#include "ge/util/Bool.h"
#include "ge/util/UInt32.h"
#include "ge/util/UtilData.h"

#include <algorithm>
#include <cassert>
#include <cstring>

struct cstr_and_len
{
    const char* cstr;
    size_t length;
};

#define CSTR_AND_LEN(A) {(A), sizeof(A)}

// Minimum value of an int32 in bases 2-16 as c-strings
static const cstr_and_len int32_min_strings[17] = {
    {0, 0},
    {0, 0},
    CSTR_AND_LEN("-10000000000000000000000000000000"),
    CSTR_AND_LEN("-12112122212110202102"),
    CSTR_AND_LEN("-2000000000000000"),
    CSTR_AND_LEN("-13344223434043"),
    CSTR_AND_LEN("-553032005532"),
    CSTR_AND_LEN("-104134211162"),
    CSTR_AND_LEN("-20000000000"),
    CSTR_AND_LEN("-5478773672"),
    CSTR_AND_LEN("-2147483648"),
    CSTR_AND_LEN("-A02220282"),
    CSTR_AND_LEN("-4BB2308A8"),
    CSTR_AND_LEN("-282BA4AAB"),
    CSTR_AND_LEN("-1652CA932"),
    CSTR_AND_LEN("-C87E66B8"),
    CSTR_AND_LEN("-80000000")};


String Int32::int32ToString(int32 value, uint32 radix)
{
    assert(radix >= 2 && radix <= 16);

    char buf[34]; // Worst case is base 2 of minimum value
    uint32 usedLen = int32ToBuffer(buf, sizeof(buf), value, radix);
    return String(buf, usedLen);
}

uint32 Int32::int32ToBuffer(char* buffer, uint32 bufferLen, int32 value, uint32 radix)
{
    // Handle minimum value that can't be multiplied by -1
    if (value == INT32_MIN)
    {
        const cstr_and_len* data = &int32_min_strings[radix];
        if (bufferLen >= data->length)
        {
            ::memcpy(buffer, data->cstr, data->length);
        }
        return data->length;
    }

    // If not INT32_MIN, we can safely multiply by -1 and use the UInt32 logic
    if (value < 1)
    {
        uint32 ret = 1 + UInt32::uint32ToBuffer(buffer+1, bufferLen, (uint32)(value * -1), radix);

        if (bufferLen != 0)
        {
            buffer[0] = '-';
        }

        return ret;
    }
    else
    {
        return UInt32::uint32ToBuffer(buffer, bufferLen, (uint32)value, radix);
    }
}

/*
 * This uses Java's Integer.parseInt() strategy accumulating while parsing
 * using negative values to avoid issues with INT32_MIN being smaller than
 * -INT32_MAX.
 */
int32 Int32::parseInt32(StringRef strRef, bool* ok, uint32 radix)
{
    assert(radix >= 2 && radix <= 16);

    uint32 strRefLen = strRef.length();
    const char* strRefData = strRef.data();

    int32 result = 0;
    bool negative = false;
    uint32 i = 0;

    int32 limit;
    int32 multmin;
    int32 digit;

    // Check for empty string
    if (strRefLen == 0)
    {
        Bool::setBool(ok, false);
        return 0;
    }

    // If there is a negative sign, note it and the appropriate limit value
    if (strRef.charAt(0) == '-')
    {
        if (strRefLen == 1)
        {
            // Only got '-'
            Bool::setBool(ok, false);
            return 0;
        }

        negative = true;
        limit = INT32_MIN;
        i++;
    }
    else
    {
        limit = -INT32_MAX;
    }

    // Generate a guard value which we can use to detect "too many digits"
    multmin = limit / (int32)radix;

    // Parse the first digit
    if (i < strRefLen)
    {
        digit = UtilData::g_charValue[(uint8)strRefData[i]];
        if (digit < 0 || digit >= (int32)radix)
        {
            Bool::setBool(ok, false);
            return 0;
        }
        else
        {
            result = -digit;
        }
        i++;
    }

    // Loop, parsing the remaining digits
    while (i < strRefLen)
    {
        // Accumulating negatively avoids surprises near INT32_MAX
        digit = UtilData::g_charValue[(uint8)strRefData[i]];
        if (digit < 0 || digit >= (int32)radix)
        {
            Bool::setBool(ok, false);
            return 0;
        }

        // Check if this will produce too many digits
        if (result < multmin)
        {
            Bool::setBool(ok, false);
            return 0;
        }

        result *= radix;

        // Check if this will overflow the maximum value
        if (result < limit + digit)
        {
            Bool::setBool(ok, false);
            return 0;
        }
        result -= digit;
        i++;
    }

    Bool::setBool(ok, true);

    if (negative)
    {
        return result;
    }
    else
    {
        return -result;
    }
}

// Int64.cpp

#include "ge/util/Int64.h"

#include "ge/util/Bool.h"
#include "ge/util/UInt64.h"
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

// Minimum value of an int64 in bases 2-16 as c-strings
static const cstr_and_len int64_min_strings[17] = {
    {0, 0},
    {0, 0},
    CSTR_AND_LEN("-1000000000000000000000000000000000000000000000000000000000000000"),
    CSTR_AND_LEN("-2021110011022210012102010021220101220102"),
    CSTR_AND_LEN("-20000000000000000000000000000000"),
    CSTR_AND_LEN("-1104332401304422434310310040"),
    CSTR_AND_LEN("-1540241003031030222122212"),
    CSTR_AND_LEN("-22341010611245052050614"),
    CSTR_AND_LEN("-1000000000000000000000"),
    CSTR_AND_LEN("-67404283172107811766"),
    CSTR_AND_LEN("-9223372036854775808"),
    CSTR_AND_LEN("-1728002635214590966"),
    CSTR_AND_LEN("-41A792678515120368"),
    CSTR_AND_LEN("-10B269549075433515"),
    CSTR_AND_LEN("-4340724C6C71DC7A8"),
    CSTR_AND_LEN("-160E2AD324636666D"),
    CSTR_AND_LEN("-8000000000000000")};


String Int64::int64ToString(int64 value, uint32 radix)
{
    assert(radix >= 2 && radix <= 16);

    char buf[66]; // Worst case is base 2 of minimum value
    uint32 usedLen = int64ToBuffer(buf, sizeof(buf), value, radix);
    return String(buf, usedLen);
}

uint32 Int64::int64ToBuffer(char* buffer, uint32 bufferLen, int64 value, uint32 radix)
{
    assert(radix >= 2 && radix <= 16);

    // Handle minimum value that can't be multiplied by -1
    if (value == INT64_MIN)
    {
        const cstr_and_len* data = &int64_min_strings[radix];
        if (bufferLen >= data->length)
        {
            ::memcpy(buffer, data->cstr, data->length);
        }
        return data->length;
    }

    // If not INT32_MIN, we can safely multiply by -1 and use the UInt32 logic
    if (value < 1)
    {
        uint32 ret = 1 + UInt64::uint64ToBuffer(buffer+1, bufferLen, (uint64)(value * -1), radix);

        if (bufferLen != 0)
        {
            buffer[0] = '-';
        }

        return ret;
    }
    else
    {
        return UInt64::uint64ToBuffer(buffer, bufferLen, (uint64)value, radix);
    }
}

/*
 * This uses Java's Integer.parseInt() strategy accumulating while parsing
 * using negative values to avoid issues with INT32_MIN being smaller than
 * -INT32_MAX.
 */
int64 Int64::parseInt64(StringRef strRef, bool* ok, uint32 radix)
{
    assert(radix >= 2 && radix <= 16);

    uint32 strRefLen = strRef.length();
    const char* strRefData = strRef.data();

    int64 result = 0;
    bool negative = false;
    uint32 i = 0;

    int64 limit;
    int64 multmin;
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
        limit = INT64_MIN;
        i++;
    }
    else
    {
        limit = -INT64_MAX;
    }

    // Generate a guard value which we can use to detect "too many digits"
    multmin = limit / (int64)radix;

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

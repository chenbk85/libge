// INetUtil.cpp

#include <ge/inet/INetUtil.h>

#include <ge/util/UInt8.h>
#include <ge/util/UInt16.h>
#include <ge/util/UInt32.h>
#include <ge/util/UInt64.h>
#include <ge/util/UtilData.h>


namespace INetUtil
{

bool ipv4AddrStringToBinary(StringRef src, uint8* dest)
{
    int32 periodCount = 0;
    size_t segmentStart = 0;
    size_t periodIndex;
    bool parseOk;
    StringRef segments[4];

    if (src.length() == 0)
    {
        return false;
    }

    // Break the string into segments based on locations of periods
    periodIndex = src.indexOf(".");

    while (periodIndex != -1)
    {
        if (periodCount == 3)
        {
            return false;
        }

        segments[periodCount] = src.substring(segmentStart, periodIndex);
        periodCount++;
        segmentStart = periodIndex + 1;
        periodIndex = src.indexOf(".", periodIndex+1);
    }

    segments[periodCount] = src.substring(segmentStart, src.length());

    long val;

    // Parse based on period locations
    switch (periodCount)
    {
    case 0:
        // If no period, the value is a raw base 10 value
        val = UInt32::parseUInt32(segments[0], &parseOk);

        if (!parseOk)
            return false;

        dest[0] = (char)((val >> 24) & 0xff);
        dest[1] = (char)((val >> 16) & 0xff);
        dest[2] = (char)((val >> 8) & 0xff);
        dest[3] = (char)(val & 0xff);
        break;
    case 1:
        // If one period, it is a single byte value and a 24 bit value.
        // Intended for specifying class A addresses.
        val = UInt8::parseUInt8(segments[0], &parseOk);

        if (!parseOk)
            return false;

        dest[0] = (char)(val & 0xff);

        val = UInt32::parseUInt32(segments[1], &parseOk);

        if (!parseOk || val > 0xffffff)
            return false;

        dest[1] = (char)((val >> 16) & 0xff);
        dest[2] = (char)((val >> 8) & 0xff);
        dest[3] = (char)(val & 0xff);
        break;
    case 2:
        // Two periods implies two 8 bit values and a 16 bit value.
        // Intended for specifying class B addresses.
        for (int i = 0; i < 2; i++)
        {
            val = UInt8::parseUInt8(segments[i], &parseOk);

            if (!parseOk)
                return false;

            dest[i] = (char)(val & 0xff);
        }

        val = UInt16::parseUInt16(segments[2], &parseOk);

        if (!parseOk)
            return false;

        dest[2] = (char)((val >> 8) & 0xff);
        dest[3] = (char)(val & 0xff);
        break;
    case 3:
        // Normal form where each byte is given individually
        for (int i = 0; i < 4; i++)
        {
            val = UInt8::parseUInt8(segments[i], &parseOk);

            if (!parseOk)
                return false;

            dest[i] = (char)(val & 0xff);
        }
        break;
    default:
        return false;
    }

    return true;
}

bool ipv6AddrStringToBinary(StringRef src, uint8* dest)
{
    uint64 srcLen = src.length();

    // Shortest valid string is "::", hence at least 2 chars
    if (srcLen < 2)
    {
        return false;
    }

    const char* strData = src.data();
    char ch;

    // Stop parsing at any percent sign
    int64 percentIndex = src.indexOf("%");

    // But don't accept nothing but a percent
    if (percentIndex == (int64)(srcLen - 1))
    {
        return false;
    }

    if (percentIndex != -1)
    {
        srcLen = percentIndex;
    }

    uint64 i = 0;

    // Handle leading ::
    if (strData[i] == ':')
    {
        i++;
        if (strData[i] != ':')
        {
            return false;
        }
    }

    int64 curtok = i;
    int64 colonPos = -1;
    int64 j = 0;
    bool haveSeenHex = false;
    uint32 val = 0;

    // Main loop parsing : delimited sections
    while (i < srcLen)
    {
        ch = strData[i];
        i++;

        // Get the value of the character as hex
        int8 chval = UtilData::g_charValue[(uint8)ch];
        if (chval != -1)
        {
            // Shift existing value and or on new value
            val <<= 4;
            val |= chval;
            if (val > 0xffff)
            {
                return false;
            }
            haveSeenHex = true;
            continue;
        }

        // If it wasn't a hex character, it better be a colon
        if (ch != ':')
            return false;

        // If we do see a colon, make sure it was in a valid spot
        curtok = i;
        if (!haveSeenHex)
        {
            if (colonPos != -1)
            {
                return false;
            }
            colonPos = j;
            continue;
        }
        else if (i == srcLen)
        {
            return false;
        }

        if (j + 2 > 16)
        {
            return false;
        }

        dest[j++] = (char)((val >> 8) & 0xff);
        dest[j++] = (char)(val & 0xff);
        haveSeenHex = false;
        val = 0;
    }

    if (haveSeenHex)
    {
        if (j + 2 > 16)
            return false;

        dest[j++] = (char)((val >> 8) & 0xff);
        dest[j++] = (char)(val & 0xff);
    }

    if (colonPos != -1)
    {
        uint64 n = j - colonPos;

        if (j == 16)
            return false;

        for (i = 1; i <= n; i++)
        {
            dest[16 - i] = dest[colonPos + n - i];
            dest[colonPos + n - i] = 0;
        }

        j = 16;
    }
    
    if (j != 16)
        return false;

    return true;
}

} // End namespace INetUtil

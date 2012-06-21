// HttpUtil.cpp

#include "ge/http/HttpUtil.h"

#include "ge/data/ShortList.h"

/* Table of safe URL characters that do not need to be escaped
 * 0-9,a-z,A-Z
 */
static
char safeUrlChars[256] =
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 00-0F */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 10-1F */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 20-2F */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,  /* 30-3F */
     0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 40-4F */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,  /* 50-5F */
     0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  /* 60-6F */
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,  /* 70-7F */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 80-8F */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* 90-9F */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* A0-AF */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* B0-BF */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* C0-CF */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* D0-DF */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  /* E0-EF */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; /* F0-FF */

// Table of hex value of a single character or -1 if invalid hex character
static
char hexValue[256] =
    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 00-0F */
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 10-1F */
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 20-2F */
      0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,  /* 30-3F */
     -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 40-4F */
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 50-5F */
     -1,10,11,12,13,14,15,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 60-6F */
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 70-7F */
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 80-8F */
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* 90-9F */
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* A0-AF */
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* B0-BF */
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* C0-CF */
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* D0-DF */
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,  /* E0-EF */
     -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}; /* F0-FF */

// RFC822 day of week strings
static
char* shortDayOfWeek[] = {"Mon", "Tue", "Wed", "Thu",
                          "Fri", "Sat", "Sun"};

// RFC822 month strings
static
char* shortMonth[] = {"Jan", "Feb", "Mar", "Apr",
                      "May", "Jun", "Jul", "Aug",
                      "Sep", "Oct", "Nov", "Dec"};

namespace HttpUtil
{

String escapeUrl(const StringRef& str)
{
    String res;
    size_t strPos = 0;
    size_t strLen = str.length();

    while (strPos < strLen)
    {
        unsigned char uc = (unsigned char)str.charAt(strPos);

        if (safeUrlChars[uc])
        {
            res.appendChar(uc);
        }
        else if (uc == ' ')
        {
            res.appendChar('+');
        }
        else
        {
            unsigned char hexHigh = uc / 16;
            unsigned char hexLow  = uc % 16;

            char hex1 = hexHigh > 9 ? hexHigh + ('A'-10) : hexHigh + '0';
            char hex2 = hexLow > 9 ? hexLow + ('A'-10) : hexLow + '0';

            res.appendChar('%');
            res.appendChar(hex1);
            res.appendChar(hex2);
        }

        strPos++;
    }

    return res;
}

bool unescapeUrl(const StringRef& str, String& dest)
{
    size_t strLen = str.length();
    ShortList<char, 256> buffer(strLen);

    buffer.addBlockBack(str.data(), strLen);

    // Delegate to the in-place function
    if (!unescapeUrlInPlace(buffer.data(), &strLen))
        return false;

    dest.append(buffer.data(), strLen);
    return true;
}

bool unescapeUrlInPlace(char* str, size_t* size)
{
    char* readPos;
    char* writePos;
    uint32 numericValue;
    uint32 hexHigh;
    uint32 hexLow;

    // This loop depends on writePos being <= readPos
    readPos  = str;
    writePos = str;

    while (*readPos)
    {
        switch (*readPos)
	    {
	    case '%':
            if (readPos[1] == '\0' ||
                readPos[2] == '\0')
            {
                return false;
            }

            hexHigh = hexValue[(unsigned char)readPos[1]];
            hexLow = hexValue[(unsigned char)readPos[2]];

            if (hexHigh < 0 || hexLow < 0)
            {
                return false;
            }

	        numericValue = (hexHigh * 16) + hexLow;
            *writePos = (char)numericValue;
            readPos += 3;
            break;

        case '+':
	        *writePos = ' ';
            readPos++;
	        break;

	    default:
	        *writePos = *readPos;
            readPos++;
	    }

        writePos++;
    }

    *writePos = '\0'; // add 0-terminator
    return true;
}

String formatTimestamp(Date date)
{
    char buffer[30];
    formatTimestamp(date, buffer);
    return String(buffer, 30);
}

void formatTimestamp(Date date,
                     char* dest)
{
    const char* shortDay;
    const char* shortMon;

    // TODO: Rework to be more efficient
    uint32 year = date.getYear();
    uint32 month = date.getMonth();
    uint32 dayOfWeek = date.getDayOfWeek();
    uint32 dayOfMonth = date.getDayOfMonth();
    uint32 hour = date.getHour();
    uint32 minute = date.getMinute();
    uint32 second = date.getSecond();

    shortDay = shortDayOfWeek[dayOfWeek];
    shortMon = shortMonth[month];

    // Format like: "Tue, 13 Feb 2012 19:44:06 GMT"
    dest[0]  = shortDay[0];
    dest[1]  = shortDay[1];
    dest[2]  = shortDay[2];
    dest[3]  = ',';
    dest[4]  = ' ';
    dest[5]  = '0' + (dayOfMonth / 10);
    dest[6]  = '0' + (dayOfMonth % 10);
    dest[7]  = ' ';
    dest[8]  = shortMon[0];
    dest[9]  = shortMon[1];
    dest[10] = shortMon[2];
    dest[11] = ' ';
    dest[12]  = '0' + (year / 1000);
    dest[13]  = '0' + ((year % 1000) / 100);
    dest[14]  = '0' + ((year % 100) / 10);
    dest[15]  = '0' + (year % 10);
    dest[16] = ' ';
    dest[17] = '0' + (hour / 10);
    dest[18] = '0' + (hour % 10);
    dest[19] = ':';
    dest[20] = '0' + (minute / 10);
    dest[21] = '0' + (minute % 10);
    dest[22] = ':';
    dest[23] = '0' + (second / 10);
    dest[24] = '0' + (second % 10);
    dest[25] = ' ';
    dest[26] = 'G';
    dest[27] = 'M';
    dest[28] = 'T';
    dest[29] = '\0';
}

} // End namespace HttpUtil

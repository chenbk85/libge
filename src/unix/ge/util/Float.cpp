// Float.cpp

#include "ge/util/Float.h"

#include "ge/util/Bool.h"
#include "ge/util/Int32.h"

#include <cassert>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <vector>


static void makeFormatStr(char* buffer,
                          char format,
                          int32 precision);

String Float::floatToString(float value,
                            char format,
                            int32 precision)
{
    assert(format == 'e' ||
           format == 'E' ||
           format == 'f' ||
           format == 'g' ||
           format == 'G');

    std::stringstream ss;
    ss.imbue(std::locale("C"));
    ss << value;
    std::string str = ss.str();
    return String(str.c_str(), str.length());
}

uint32 Float::floatToBuffer(char* buffer,
                            uint32 bufferLen,
                            float value,
                            char format,
                            int32 precision)
{
    assert(format == 'e' ||
           format == 'E' ||
           format == 'f' ||
           format == 'g' ||
           format == 'G');

    std::stringstream ss;
    ss.imbue(std::locale("C"));
    ss << value;
    std::string str = ss.str();
    uint32 strLen = str.length();

    if (strLen < bufferLen)
    {
        ::memcpy(buffer, str.c_str(), strLen);
    }

    return strLen;
}

float Float::parseFloat(StringRef strRef, bool* ok)
{
    char buffer[256];
    char* ptr;
    float ret;
    
    ptr = buffer;

    std::stringstream ss;
    ss.imbue(std::locale("C"));
    ss << ptr;
    ss >> ret;

    // Have to check if there is anything left in the string
    bool success = (!ss.fail() && ss.str().length() == 0);

    if (!success)
    {
        ret = 0;
    }

    Bool::setBool(ok, success);
    return ret;
}

String Float::localeFloatToString(float value,
                                  char format,
                                  int32 precision)
{
    char buf[512];
    char formatStr[14];

    // Generate a format string
    makeFormatStr(formatStr, format, precision);

    uint32 printLen = (uint32)::snprintf(buf, sizeof(buf), formatStr, value);

    if (printLen < sizeof(buf))
    {
        return String(buf);
    }
    else
    {
        std::vector<char> tempBuf(printLen+1);
        ::snprintf(&tempBuf[0], printLen+1, formatStr, value);
        return String(&tempBuf[0], printLen);
    }
}

uint32 Float::localeFloatToBuffer(char* buffer,
                                  uint32 bufferLen,
                                  float value,
                                  char format,
                                  int32 precision)
{
    assert(format == 'e' ||
           format == 'E' ||
           format == 'f' ||
           format == 'g' ||
           format == 'G');

    char tempBuffer[512];
    char formatStr[14];

    // Generate a format string
    makeFormatStr(formatStr, format, precision);

    uint32 printLen = (uint32)::snprintf(tempBuffer, sizeof(tempBuffer), formatStr, value);

    if (printLen < sizeof(tempBuffer))
    {
        ::memcpy(buffer, tempBuffer, printLen);
    }
    else if (printLen < bufferLen)
    {
        ::snprintf(buffer, printLen, formatStr, value);
    }

    return printLen;
}

float Float::parseLocaleFloat(StringRef strRef, bool* ok)
{
    char buffer[256];
    char* ptr;
    char* endPtr;
    float ret;
    uint32 strRefLen = strRef.length();
    
    ptr = buffer;

    if (strRefLen >= sizeof(buffer))
    {
        ptr = new char[strRefLen+1];
    }

    ::memcpy(ptr, strRef.data(), strRefLen);
    ptr[strRefLen] = '\0';

    errno = 0;
    ret = (float)::strtod(ptr, &endPtr);

    bool success = (errno == 0 && endPtr == ptr + strRefLen);

    if (!success)
    {
        ret = 0;
    }

    Bool::setBool(ok, success);

    return ret;
}

// Local Functions ----------------------------------------------------------

static void makeFormatStr(char buffer[14],
                          char format,
                          int32 precision)
{
    buffer[0] = '%';

    if (precision < 0)
    {
        buffer[1] = format;
        buffer[2] = '\0';
    }
    else
    {
        buffer[1] = '.';
        uint32 precLen = Int32::int32ToBuffer(buffer+2, precision, 10);
        buffer[2+precLen] = format;
        buffer[3+precLen] = '\0';
    }
}

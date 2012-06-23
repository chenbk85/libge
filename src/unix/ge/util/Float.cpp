// Float.cpp

#include "ge/util/Float.h"

#include "ge/util/Bool.h"
#include "ge/util/Int32.h"

#include <cassert>
#include <cstring>
#include <sstream>

// TODO: Need to apply format flags

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
    ss.precision(precision);
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
    ss.precision(precision);
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
    std::stringstream ss;
    ss.precision(precision);
    ss << value;
    std::string str = ss.str();
    return String(str.c_str(), str.length());
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

    std::stringstream ss;
    ss.precision(precision);
    ss << value;
    std::string str = ss.str();
    uint32 strLen = str.length();

    if (strLen < bufferLen)
    {
        ::memcpy(buffer, str.c_str(), strLen);
    }

    return strLen;
}

float Float::parseLocaleFloat(StringRef strRef, bool* ok)
{
    std::stringstream ss;
    float ret;

    ss.write(strRef.data(), strRef.length());
    ss >> ret;

    // Check that parsing did not fail and that every character was used
    if (!ss.fail() && (ss.tellg() == (std::streampos)strRef.length()))
    {
        Bool::setBool(ok, true);
        return ret;
    }
    else
    {
        Bool::setBool(ok, false);
        return 0.0f;
    }
}

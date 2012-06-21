// Double.cpp

#include <ge/util/Double.h>

#include <ge/data/ShortList.h>
#include <ge/util/Bool.h>
#include <ge/util/Int32.h>
#include <gepriv/WinUtil.h>

#include <cassert>
#include <clocale>
#include <cstdio>
#include <sstream>


static void makeFormatStr(char* buffer,
                          char format,
                          int32 precision);

String Double::doubleToString(double value,
                              char format,
                              int32 precision)
{
    ShortList<char, 512> textList(512);
    char formatStr[14];

    // Generate a format string
    makeFormatStr(formatStr, format, precision);

    _locale_t cLocale = ::_create_locale(LC_NUMERIC, "C");

    textList.uninitializedResize(512);
    uint32 printLen = (uint32)::_snprintf_l(textList.data(), 512, formatStr, cLocale, value);

    if (printLen >= 512)
    {
        textList.uninitializedResize(printLen+1);
        ::_snprintf_l(textList.data(), printLen+1, formatStr, cLocale, value);
    }

    ::_free_locale(cLocale);
    return String(textList.data(), printLen);
}

uint32 Double::doubleToBuffer(char* buffer,
                              uint32 bufferLen,
                              double value,
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

    _locale_t cLocale = ::_create_locale(LC_NUMERIC, "C");

    uint32 printLen = (uint32)::_snprintf_l(tempBuffer, sizeof(tempBuffer), formatStr, cLocale, value);

    if (printLen < sizeof(tempBuffer))
    {
        ::memcpy(buffer, tempBuffer, printLen);
    }
    else if (printLen < bufferLen)
    {
        ::_snprintf_l(buffer, printLen, formatStr, cLocale, value);
    }

    ::_free_locale(cLocale);
    return printLen;
}

double Double::parseDouble(StringRef strRef, bool* ok)
{
    std::stringstream ss;
    ss.imbue(std::locale("C"));
    double ret;

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
        return 0.0;
    }
}

String Double::localeDoubleToString(double value,
                                    char format,
                                    int32 precision)
{
    ShortList<char, 512> textList(512);
    char formatStr[14];

    // TODO: Broken due to Windows _snprintf result
    // Generate a format string
    makeFormatStr(formatStr, format, precision);

    textList.uninitializedResize(512);
    uint32 printLen = (uint32)::_snprintf(textList.data(), 512, formatStr, value);

    if (printLen >= 512)
    {
        textList.uninitializedResize(printLen+1);
        ::_snprintf(textList.data(), printLen+1, formatStr, value);
    }

    return String(textList.data(), printLen);
}

uint32 Double::localeDoubleToBuffer(char* buffer,
                                    uint32 bufferLen,
                                    double value,
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

    uint32 printLen = (uint32)::_snprintf(tempBuffer, sizeof(tempBuffer), formatStr, value);

    if (printLen < sizeof(tempBuffer))
    {
        ::memcpy(buffer, tempBuffer, printLen);
    }
    else if (printLen < bufferLen)
    {
        ::_snprintf(buffer, printLen, formatStr, value);
    }

    return printLen;
}

double Double::parseLocaleDouble(StringRef strRef, bool* ok)
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
        return 0.0;
    }
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

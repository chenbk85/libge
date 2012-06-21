// Double.cpp

#include "ge/util/Double.h"

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

String Double::doubleToString(double value,
                              char format,
                              int32 precision)
{
    assert(format == 'e' ||
           format == 'E' ||
           format == 'f' ||
           format == 'g' ||
           format == 'G');

    // This default implementation works, but suffers a bit in performance
    std::stringstream ss;
    ss.imbue(std::locale("C"));
    ss << value;
    std::string str = ss.str();
    return String(str.c_str(), str.length());
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

    // This default implementation works, but suffers a bit in performance
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

double Double::parseLocaleDouble(StringRef strRef, bool* ok)
{
	std::stringstream ss;
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

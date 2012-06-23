// Double.cpp

#include "ge/util/Double.h"

#include "ge/util/Bool.h"
#include "ge/util/Int32.h"

#include <cassert>
#include <cstring>
#include <sstream>

// TODO: Need to apply format flags

String Double::doubleToString(double value,
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
    std::stringstream ss;
    ss.precision(precision);
    ss << value;
    std::string str = ss.str();
    return String(str.c_str(), str.length());
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
		return 0.0d;
	}
}

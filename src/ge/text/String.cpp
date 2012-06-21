// String.cpp

#include <ge/text/String.h>
#include <ge/text/StringRef.h>
#include <ge/text/UnicodeUtil.h>

#include <ge/util/Double.h>
#include <ge/util/Float.h>
#include <ge/util/Int32.h>
#include <ge/util/Int64.h>
#include <ge/util/UInt32.h>
#include <ge/util/UInt64.h>

#include <algorithm>
#include <cassert>
#include <cstring>

#define SMALL_STR_LEN (sizeof(size_t) * 2 + sizeof(char*))
#define SMALL_STR_MAX SMALL_STR_LEN-1

String::String() :
    len(0),
    reserved(SMALL_STR_LEN),
    strData(shortStr)
{
}

String::String(const String& str)
{
    len = str.len;

    if (str.len < SMALL_STR_MAX)
    {
        strData = shortStr;
        reserved = SMALL_STR_LEN;
        ::memcpy(shortStr, str.shortStr, str.len+1);
    }
    else
    {
        size_t newLen = str.len+1;
        reserved = newLen;
        strData = new char[newLen];
        ::memcpy(strData, str.strData, newLen);
    }
}

#if defined(HAVE_RVALUE)
String::String(String&& str)
{
    len = str.len;

    if (str.len < SMALL_STR_MAX)
    {
        strData = shortStr;
        reserved = SMALL_STR_LEN;
        ::memcpy(shortStr, str.shortStr, str.len+1);
    }
    else
    {
        reserved = str.reserved;
        strData = str.strData;
        str.len = 0;
    }
}
#endif

String::String(const StringRef strRef)
{
    len = strRef.length();

    if (len < SMALL_STR_MAX)
    {
        strData = shortStr;
        reserved = SMALL_STR_LEN;
        ::memcpy(shortStr, strRef.data(), len);
        shortStr[len] = '\0';
    }
    else
    {
        size_t refLen = strRef.length();
        size_t newLen = refLen+1;
        reserved = newLen;
        strData = new char[newLen];
        ::memcpy(strData, strRef.data(), refLen);
        strData[refLen] = '\0';
    }
}

#if defined(HAVE_RVALUE)
String::String(const StringRef&& strRef)
{
    len = strRef.length();

    if (len < SMALL_STR_MAX)
    {
        strData = shortStr;
        reserved = SMALL_STR_LEN;
        ::memcpy(shortStr, strRef.data(), len);
        shortStr[len] = '\0';
    }
    else
    {
        size_t refLen = strRef.length();
        size_t newLen = refLen+1;
        reserved = newLen;
        strData = new char[newLen];
        ::memcpy(strData, strRef.data(), refLen);
        strData[refLen] = '\0';
    }
}
#endif

String::String(const char* cstr)
{
    len = ::strlen(cstr);

    if (len < SMALL_STR_MAX)
    {
        strData = shortStr;
        reserved = SMALL_STR_LEN;
        ::memcpy(shortStr, cstr, len+1);
    }
    else
    {
        size_t newLen = len+1;
        reserved = newLen;
        strData = new char[newLen];
        ::memcpy(strData, cstr, newLen);
    }
}

String::String(const char* cstr, size_t dataLen)
{
    len = dataLen;

    if (len < SMALL_STR_MAX)
    {
        strData = shortStr;
        reserved = SMALL_STR_LEN;
        ::memcpy(shortStr, cstr, dataLen);
        shortStr[len] = '\0';
    }
    else
    {
        size_t newLen = dataLen+1;
        reserved = newLen;
        strData = new char[newLen];
        ::memcpy(strData, cstr, dataLen);
        strData[len] = '\0';
    }
}

String& String::operator=(const String& str)
{
    if (this != &str)
    {
        if (len >= SMALL_STR_MAX)
        {
            delete[] strData;
        }

        len = str.len;

        if (str.len < SMALL_STR_MAX)
        {
            strData = shortStr;
            reserved = SMALL_STR_LEN;
            ::memcpy(shortStr, str.shortStr, str.len+1);
        }
        else
        {
            size_t newLen = str.len+1;
            reserved = newLen;
            strData = new char[newLen];
            ::memcpy(strData, str.strData, newLen);
        }
    }

    return *this;
}

#if defined(HAVE_RVALUE)
String& String::operator=(String&& str)
{
    if (this != &str)
    {
        if (len >= SMALL_STR_MAX)
        {
            delete[] strData;
        }

        len = str.len;

        if (str.len < SMALL_STR_MAX)
        {
            strData = shortStr;
            reserved = SMALL_STR_LEN;
            ::memcpy(shortStr, str.shortStr, str.len+1);
        }
        else
        {
            reserved = str.reserved;
            strData = str.strData;
            str.len = 0;
        }
    }

    return *this;
}

#endif

String& String::operator=(const StringRef& strRef)
{
    // Because it's possible to assign a StringRef that is a substring of
    // this String, we have to use memmove instead of memcpy and defer
    // deletion of the old data until the end.
    const char* oldData = NULL;

    if (len >= SMALL_STR_MAX)
    {
        oldData = strData;
    }

    len = strRef.length();

    if (len < SMALL_STR_MAX)
    {
        strData = shortStr;
        reserved = SMALL_STR_LEN;
        ::memmove(shortStr, strRef.data(), len);
        shortStr[len] = '\0';
    }
    else
    {
        size_t newLen = len+1;
        reserved = newLen;
        strData = new char[newLen];
        ::memmove(strData, strRef.data(), len);
        strData[len] = '\0';
    }

    delete[] oldData;

    return *this;
}

String& String::operator=(const char* cstr)
{
    if (len >= SMALL_STR_MAX)
    {
        delete[] strData;
    }

    len = ::strlen(cstr);

    if (len < SMALL_STR_MAX)
    {
        strData = shortStr;
        reserved = SMALL_STR_LEN;
        ::memcpy(shortStr, cstr, len+1);
    }
    else
    {
        size_t newLen = len+1;
        reserved = newLen;
        strData = new char[newLen];
        ::memcpy(strData, cstr, newLen);
    }

    return *this;
}

void String::append(const StringRef strRef)
{
    append_raw(strRef.data(), strRef.length());
}

void String::append(const char* data, size_t dataLen)
{
    append_raw(data, dataLen);
}

void String::appendChar(char c)
{
    append_raw(&c, 1);
}

void String::appendCodePoint(utf32 cp)
{
    assert(UnicodeUtil::isValidCodePoint(cp) && "Invalid code point");

    char buf[4];
    size_t len = UnicodeUtil::utf8FromCodePoint(cp, buf);
    append_raw(buf, len);
}

void String::appendInt8(int8 value)
{
    char buffer[4];
    size_t filled = Int32::int32ToBuffer(buffer, sizeof(buffer), (int32)value, 10);
    append(StringRef(buffer, filled));
}

void String::appendInt16(int16 value)
{
    char buffer[6];
    size_t filled = Int32::int32ToBuffer(buffer, sizeof(buffer), (int32)value, 10);
    append(StringRef(buffer, filled));
}

void String::appendInt32(int32 value)
{
    char buffer[11];
    size_t filled = Int32::int32ToBuffer(buffer, sizeof(buffer), value, 10);
    append(StringRef(buffer, filled));
}

void String::appendInt64(int64 value)
{
    char buffer[20];
    size_t filled = Int64::int64ToBuffer(buffer, sizeof(buffer), value, 10);
    append(StringRef(buffer, filled));
}

void String::appendUInt8(uint8 value)
{
    char buffer[3];
    size_t filled = UInt32::uint32ToBuffer(buffer, sizeof(buffer), (uint32)value, 10);
    append(StringRef(buffer, filled));
}

void String::appendUInt16(uint16 value)
{
    char buffer[5];
    size_t filled = UInt32::uint32ToBuffer(buffer, sizeof(buffer), (uint32)value, 10);
    append(StringRef(buffer, filled));
}

void String::appendUInt32(uint32 value)
{
    char buffer[10];
    size_t filled = UInt32::uint32ToBuffer(buffer, sizeof(buffer), value, 10);
    append(StringRef(buffer, filled));
}

void String::appendUInt64(uint64 value)
{
    char buffer[20];
    size_t filled = UInt64::uint64ToBuffer(buffer, sizeof(buffer), value, 10);
    append(StringRef(buffer, filled));
}

void String::appendFloat(float value)
{
    append(Float::floatToString(value));
}

void String::appendDouble(double value)
{
    append(Double::doubleToString(value));
}

void String::append_raw(const char* appendData, size_t dataLen)
{
    if (len + dataLen < reserved)
    {
        ::memcpy(strData+len, appendData, dataLen);
        strData[len+dataLen] = '\0';
    }
    else
    {
        size_t newLen = len + dataLen + 1;
        size_t newReserved = newLen * 2;
        char* newData = new char[newReserved];
        ::memcpy(newData, strData, len);

        if (strData != shortStr)
        {
            delete[] strData;
        }

        ::memcpy(newData + len, appendData, dataLen);
        newData[len + dataLen] = '\0';

        reserved = newReserved;
        strData = newData;
    }

    len += dataLen;
}

void String::append_cstr(const char* cstr, size_t cstrLen)
{
    if (len + cstrLen < SMALL_STR_MAX)
    {
        ::memcpy(shortStr+len, cstr, cstrLen+1);
    }
    else
    {
        size_t newLen = len + cstrLen + 1;
        size_t newReserved = newLen * 2;
        char* newData = new char[newReserved];

        if (len < SMALL_STR_MAX)
        {
            ::memcpy(newData, shortStr, len);
        }
        else
        {
            ::memcpy(newData, strData, len);
            delete[] strData;
        }

        ::memcpy(newData + len, cstr, cstrLen+1);

        reserved = newReserved;
        strData = newData;
    }

    len += cstrLen;
}

const char* String::c_str() const
{
    return data();
}

const char* String::data() const
{
    if (len < SMALL_STR_MAX)
    {
        return shortStr;
    }
    else
    {
        return strData;
    }
}

uint32 String::hash() const
{
    return StringRef(*this).hash();
}

const char String::charAt(size_t pos) const
{
    if (len < SMALL_STR_MAX)
    {
        return shortStr[pos];
    }
    else
    {
        return strData[pos];
    }
}

const utf32 String::codePointAt(size_t pos) const
{
    const char* dataPtr = data();
    return UnicodeUtil::nextUtf8CodePoint(&dataPtr);
}

int32 String::compare(const StringRef& strRef) const
{
    size_t strRefLen = strRef.length();
    const char* localData = data();
    const char* strRefData = strRef.data();

    // Emulate behavior of strcmp. That is, do comparison as if the StringRef
    // was nul terminated.
    if (len == strRefLen)
    {
        return ::memcmp(localData, strRefData, len);
    }
    else if (len < strRefLen)
    {
        int32 partial = ::memcmp(localData, strRefData, len);

        if (partial != 0)
            return partial;
        return 1;
    }
    else
    {
        int32 partial = ::memcmp(localData, strRefData, strRefLen);

        if (partial != 0)
            return partial;
        return -1;
    }
}

uint32 String::editDistance(const StringRef& strRef) const
{
    // Just delegate to StringRef
    return StringRef(*this).editDistance(strRef);
}

ssize_t String::indexOf(const StringRef& strRef) const
{
    return indexOf(strRef, 0);
}

ssize_t String::indexOf(const StringRef& strRef, size_t startIndex) const
{
    return StringRef(*this).indexOf(strRef, startIndex);
}

ssize_t String::lastIndexOf(const StringRef& strRef) const
{
    return lastIndexOf(strRef, len);
}

ssize_t String::lastIndexOf(const StringRef& strRef, size_t endIndex) const
{
    return StringRef(*this).lastIndexOf(strRef, endIndex);
}

size_t String::length() const
{
    return len;
}

StringRef String::substring(size_t startIndex) const
{
    return StringRef(data()+startIndex, len-startIndex);
}

StringRef String::substring(size_t startIndex, size_t endIndex) const
{
    return StringRef(data()+startIndex, endIndex-startIndex);
}

bool String::startsWith(const StringRef& strRef) const
{
    if (len < strRef.length())
    {
        return false;
    }

    return ::memcmp(data(), strRef.data(), strRef.length()) == 0;
}

bool String::endsWith(const StringRef& strRef) const
{
    if (len < strRef.length())
    {
        return false;
    }

    size_t refLen = strRef.length();
    return ::memcmp(data() + len - refLen, strRef.data(), refLen) == 0;
}

void String::reserve(size_t size)
{
    // Don't bother with small sizes and do nothing if already reserved
    if (size < SMALL_STR_MAX ||
        size <= reserved)
    {
        return;
    }

    char* newBuffer = new char[size+1];

    if (len > 0)
    {
        ::memcpy(newBuffer, strData, len+1);
        delete[] strData;
    }

    strData = newBuffer;
    reserved = size+1;
}

void String::trim()
{
    // TODO
}

const String operator+(const String& str1, const String& str2)
{
    String ret;
    ret.reserve(str1.length() + str2.length());
    ret.append(str1);
    ret.append(str2);
    return ret;
}

const String operator+(const StringRef& strRef, const String& str)
{
    String ret;
    ret.reserve(strRef.length() + str.length());
    ret.append(strRef);
    ret.append(str);
    return ret;
}

const String operator+(const String& str, const StringRef& strRef)
{
    String ret;
    ret.reserve(str.length() + strRef.length());
    ret.append(str);
    ret.append(strRef);
    return ret;
}

const String operator+(const char* cstr, const String& str)
{
    String ret;
    size_t cstrLen = ::strlen(cstr);
    ret.reserve(cstrLen + str.length());
    ret.append(cstr, cstrLen);
    ret.append(str);
    return ret;
}

const String operator+(const String& str, const char* cstr)
{
    String ret;
    size_t cstrLen = ::strlen(cstr);
    ret.reserve(str.length() + cstrLen);
    ret.append(str);
    ret.append(cstr, cstrLen);
    return ret;
}

const String operator+(char c, const String& str)
{
    String ret;
    ret.reserve(str.length() + 1);
    ret.appendChar(c);
    ret.append(str);
    return ret;
}

const String operator+(const String& str, char c)
{
    String ret;
    ret.reserve(str.length() + 1);
    ret.append(str);
    ret.appendChar(c);
    return ret;
}

bool operator<(const String& str1, const String& str2)
{
    return ::strcmp(str1.data(), str2.data()) < 0;
}

bool operator<(const String& str, const char* cstr)
{
    return ::strcmp(str.data(), cstr) < 0;
}

bool operator<(const char* cstr, const String& str)
{
    return ::strcmp(cstr, str.data()) < 0;
}

bool operator<=(const String& str1, const String& str2)
{
    return ::strcmp(str1.data(), str2.data()) <= 0;
}

bool operator<=(const String& str, const char* cstr)
{
    return ::strcmp(str.data(), cstr) <= 0;
}

bool operator<=(const char* cstr, const String& str)
{
    return ::strcmp(cstr, str.data()) <= 0;
}

bool operator>(const String& str1, const String& str2)
{
    return ::strcmp(str1.data(), str2.data()) > 0;
}

bool operator>(const String& str, const char* cstr)
{
    return ::strcmp(str.data(), cstr) > 0;
}

bool operator>(const char* cstr, const String& str)
{
    return ::strcmp(cstr, str.data()) > 0;
}

bool operator>=(const String& str1, const String& str2)
{
    return ::strcmp(str1.data(), str2.data()) >= 0;
}

bool operator>=(const String& str, const char* cstr)
{
    return ::strcmp(str.data(), cstr) >= 0;
}

bool operator>=(const char* cstr, const String& str)
{
    return ::strcmp(cstr, str.data()) >= 0;
}

bool operator==(const String& str1, const String& str2)
{
    return str1.compare(str2) == 0;
}

bool operator==(const char* cstr, const String& str)
{
    return str.compare(StringRef(cstr)) == 0;
}

bool operator==(const String& str, const char* cstr)
{
    return str.compare(StringRef(cstr)) == 0;
}

bool operator!=(const String& str1, const String& str2)
{
    return str1.compare(str2) != 0;
}

bool operator!=(const char* cstr, const String& str)
{
    return str.compare(StringRef(cstr)) != 0;
}

bool operator!=(const String& str, const char* cstr)
{
    return str.compare(StringRef(cstr)) != 0;
}

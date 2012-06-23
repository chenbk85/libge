// String.h

#ifndef STRING_H
#define STRING_H

#include <ge/common.h>
#include <ge/text/StringRef.h>

class String
{
public:
    String();
    String(const String& str);
    String(String&& str);
    String(const StringRef strRef);
    String(const StringRef&& strRef);
    String(const char* cstr);
    String(const char* data, size_t dataLen);

    String& operator=(const String& str);
    String& operator=(String&& str);
    String& operator=(const StringRef& strRef);
    String& operator=(const char* cstr);

    void append(const StringRef strRef);
    void append(const char* data, size_t dataLen);
    void appendChar(char c);
    void appendCodePoint(utf32 cp);

    void appendInt8(int8 value);
    void appendInt16(int16 value);
    void appendInt32(int32 value);
    void appendInt64(int64 value);
    void appendUInt8(uint8 value);
    void appendUInt16(uint16 value);
    void appendUInt32(uint32 value);
    void appendUInt64(uint64 value);
    void appendFloat(float value);
    void appendDouble(double value);

    const char* c_str() const;
    const char* data() const;

    uint32 hash() const;

    const char charAt(size_t pos) const;
    const utf32 codePointAt(size_t pos) const;

    int32 compare(const StringRef& strRef) const;

    uint32 editDistance(const StringRef& strRef) const;

    bool equals(const StringRef& strRef) const;

    ssize_t indexOf(const StringRef& strRef) const;
    ssize_t indexOf(const StringRef& strRef, size_t startIndex) const;

    ssize_t lastIndexOf(const StringRef& strRef) const;
    ssize_t lastIndexOf(const StringRef& strRef, size_t endIndex) const;

    size_t length() const;

    StringRef substring(size_t startIndex) const;
    StringRef substring(size_t startIndex, size_t endIndex) const;

    bool startsWith(const StringRef& strRef) const;

    bool endsWith(const StringRef& strRef) const;

    void reserve(size_t size);

    void trim();

    String& operator+=(const String& str);
    String& operator+=(const StringRef& strRef);
    String& operator+=(const char* cstr);
    String& operator+=(char c);

private:
    void append_raw(const char* data, size_t dataLen);
    void append_cstr(const char* cstr, size_t cstrLen);

	size_t reserved;
    size_t len;
	char* strData;

    char shortStr[32 - (sizeof(size_t) * 2 + sizeof(char*))];
};

const String operator+(const String& str1, const String& str2);
const String operator+(const StringRef& strRef, const String& str);
const String operator+(const String& str, const StringRef& strRef);
const String operator+(const char* cstr, const String& str);
const String operator+(const String& str, const char* cstr);
const String operator+(char c, const String& str);
const String operator+(const String& str, char c);

bool operator<(const String& str1, const String& str2);
bool operator<(const String& str, const char* cstr);
bool operator<(const char* cstr, const String& str);
bool operator<=(const String& str1, const String& str2);
bool operator<=(const String& str, const char* cstr);
bool operator<=(const char* cstr, const String& str);
bool operator>(const String& str1, const String& str2);
bool operator>(const String& str, const char* cstr);
bool operator>(const char* cstr, const String& str);
bool operator>=(const String& str1, const String& str2);
bool operator>=(const String& str, const char* cstr);
bool operator>=(const char* cstr, const String& str);
bool operator==(const String& str1, const String& str2);
bool operator==(const char* cstr, const String& str);
bool operator==(const String& str, const char* cstr);
bool operator!=(const String& str1, const String& str2);
bool operator!=(const char* cstr, const String& strRef);
bool operator!=(const String& str, const char* cstr);

#endif // STRING_H

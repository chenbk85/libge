// StringRef.h

#ifndef STRING_REF_H
#define STRING_REF_H

#include <ge/common.h>

class String;

class StringRef
{
public:
    StringRef();
    StringRef(const StringRef& other);
    StringRef(const String& str);
    StringRef(const char* str);
    StringRef(const char* str, size_t len);

    const char charAt(size_t pos) const;
    const utf32 codePointAt(size_t pos) const;

    int32 compare(const StringRef& strRef) const;

    const char* data() const;

    uint32 hash() const;

    uint32 editDistance(const StringRef& strRef) const;

    bool equals(const StringRef& strRef) const;

    int32 engCompareIgnoreCase(const StringRef& strRef) const;
    bool engEqualsIgnoreCase(const StringRef& strRef) const;

    ssize_t indexOf(const StringRef& strRef) const;
    ssize_t indexOf(const StringRef& strRef, size_t startIndex) const;

    ssize_t lastIndexOf(const StringRef& strRef) const;
    ssize_t lastIndexOf(const StringRef& strRef, size_t endIndex) const;

    size_t length() const;

    StringRef substring(size_t startIndex) const;
    StringRef substring(size_t startIndex, size_t endIndex) const;

    bool startsWith(const StringRef& strRef) const;

    bool endsWith(const StringRef& strRef) const;

private:
    const char* strData;
    size_t len;
};

bool operator<(const StringRef& strRef1, const StringRef& strRef2);
bool operator<(const char* cstr, const StringRef& strRef);
bool operator<(const StringRef& strRef, const char* cstr);
bool operator<=(const StringRef& strRef1, const StringRef& strRef2);
bool operator<=(const char* cstr, const StringRef& strRef);
bool operator<=(const StringRef& strRef, const char* cstr);
bool operator>(const StringRef& strRef1, const StringRef& strRef2);
bool operator>(const char* cstr, const StringRef& strRef);
bool operator>(const StringRef& strRef, const char* cstr);
bool operator>=(const StringRef& strRef1, const StringRef& strRef2);
bool operator>=(const char* cstr, const StringRef& strRef);
bool operator>=(const StringRef& strRef, const char* cstr);
bool operator==(const StringRef& strRef1, const StringRef& strRef2);
bool operator==(const char* cstr, const StringRef& strRef);
bool operator==(const StringRef& strRef, const char* cstr);
bool operator!=(const StringRef& strRef1, const StringRef& strRef2);
bool operator!=(const char* cstr, const StringRef& strRef);
bool operator!=(const StringRef& strRef, const char* cstr);

#endif // STRING_REF_H

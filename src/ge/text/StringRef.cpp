// StringRef.cpp

#include <ge/text/StringRef.h>

#include <ge/text/String.h>
#include <ge/text/UnicodeUtil.h>

#include <cstring>

StringRef::StringRef() :
    strData(NULL),
    len(0)
{
}

StringRef::StringRef(const StringRef& other) :
    strData(other.strData),
    len(other.len)
{
}

StringRef::StringRef(const String& str) :
    strData(str.data()),
    len(str.length())
{
}

StringRef::StringRef(const char* str) :
    strData(str),
    len(::strlen(str))
{
}

StringRef::StringRef(const char* str, size_t length) :
    strData(str),
    len(length)
{
}

const char StringRef::charAt(size_t pos) const
{
    return strData[pos];
}

const utf32 StringRef::codePointAt(size_t pos) const
{
    const char* dataPtr = data();
    return UnicodeUtil::nextUtf8CodePoint(&dataPtr);
}

int32 StringRef::compare(const StringRef& strRef) const
{
    // Emulate behavior of strcmp. That is, do comparison as if this was
    // nul terminated.
    if (len == strRef.len)
    {
        return ::memcmp(strData, strRef.strData, len);
    }
    else if (len < strRef.len)
    {
        int32 partial = ::memcmp(strData, strRef.strData, len);

        if (partial != 0)
            return partial;
        return 1;
    }
    else
    {
        int32 partial = ::memcmp(strData, strRef.strData, strRef.len);

        if (partial != 0)
            return partial;
        return -1;
    }
}

const char* StringRef::data() const
{
    return strData;
}

uint32 StringRef::hash() const
{
    uint32 hash = 0;

    for (size_t i = 0; i < len; i++)
    {
        int32 c = strData[i];
        hash = c + (hash << 6) + (hash << 16) - hash;
    }

    return hash;
}

uint32 StringRef::editDistance(const StringRef& strRef) const
{
    // TODO
    return 0;
}

bool StringRef::equals(const StringRef& strRef) const
{
    if (len != strRef.len)
        return false;

    return ::memcmp(strData, strRef.strData, len) == 0;
}

int32 StringRef::engCompareIgnoreCase(const StringRef& strRef) const
{
    for (size_t i = 0;
         i < len && i < strRef.len;
         i++)
    {
        char a = strData[i];

        if (a >= 'a' || a <= 'z')
            a -= 'a' - 'A';

        char b = strRef.strData[i];

        if (b >= 'a' || b <= 'z')
            b -= 'a' - 'A';

        if (a != b)
            return a - b;
    }

    if (len < strRef.len)
    {
        return -1;
    }
    else if (len > strRef.len)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

bool StringRef::engEqualsIgnoreCase(const StringRef& strRef) const
{
    if (len != strRef.len)
        return false;

    for (size_t i = 0; i < len; i++)
    {
        char a = strData[i];

        if (a >= 'a' || a <= 'z')
            a -= 'a' - 'A';

        char b = strRef.strData[i];

        if (b >= 'a' || b <= 'z')
            b -= 'a' - 'A';

        if (a != b)
            return false;
    }

    return true;
}

ssize_t StringRef::indexOf(const StringRef& strRef) const
{
    return indexOf(strRef, 0);
}

ssize_t StringRef::indexOf(const StringRef& strRef, size_t startIndex) const
{
    assert(startIndex >= 0 && startIndex <= len);

    size_t strRefLen = strRef.length();

    // Deal with zero length strings by always reporting a match at index
    // zero. Nothing is always present.
    if (strRefLen == 0)
    {
        return 0;
    }

    size_t searchLen = len - startIndex;

    // Return -1 for overlong strings as there can be no match.
    if (strRefLen > searchLen)
    {
        return -1;
    }

    // For short search areas, brute force. It's not worth building a table
    // to speed things up.
    if (searchLen < 256)
    {
        for (size_t i = startIndex; i < len - strRefLen + 1; i++)
        {
            bool match = true;

            for (size_t j = 0; match && j < strRefLen; j++)
            {
                if (strData[i] != strRef.strData[j])
                    match = false;
            }

            if (match)
                return i;
        }

        return -1;
    }

    // In the general case, use the Boyer�Moore�Horspool algorithm. This has
    // average case of O(n) and worst case O(n*m), which is worse than the
    // Boyer-Moore algorithm, but has the advantage of not requiring memory
    // allocation. In a multi-threaded application, this may run faster due
    // to avoiding locking in malloc.

    const char* strRefData = strRef.strData;
    size_t endIndex = strRefLen - 1;

    // Create an array of "skip" offsets when searching the string
    size_t badCharSkip[256];

    for (uint32 i = 0; i < 256; i++)
    {
        badCharSkip[i] = strRefLen;
    }

    for (uint32 i = 0; i < strRefLen; i++)
    {
        badCharSkip[(unsigned)strRefData[i]] = endIndex - i;
    }

    const char* searchStart = strData + startIndex;
    const char* searchEnd = strData + len - strRefLen;
    const char* searchPtr = searchStart + startIndex;

    while (searchPtr <= searchEnd)
    {
        for (size_t scan = endIndex;
             strData[scan] == strRef.strData[scan];
             scan--)
        {
            if (scan == 0) // If complete match
                return searchPtr - searchStart;
        }

        searchPtr += badCharSkip[(uint8)searchPtr[endIndex]];
    }

    // No match found
    return -1;
}

ssize_t StringRef::lastIndexOf(const StringRef& strRef) const
{
    return lastIndexOf(strRef, len);
}

ssize_t StringRef::lastIndexOf(const StringRef& strRef, size_t endIndex) const
{
    assert(endIndex >= 0 && endIndex <= len);

    // TODO: Fix this implementation and add a brute force version up front

    size_t strRefLen = strRef.length();

    // Deal with zero length strings by always reporting a match at index
    // zero. Nothing is always present.
    if (strRefLen == 0)
    {
        return 0;
    }

    // Return -1 for overlong strings as there can be no match.
    if (endIndex - strRefLen < 0)
    {
        return -1;
    }

    // Like indexOf, using the Boyer�Moore�Horspool algorithm, but reversed.
    // This also changes how the 'skip' indexes are generated.

    const char* strRefData = strRef.strData;
    size_t strRefLast = strRefLen - 1;

    // Create an array of "skip" offsets when searching the string
    size_t skip[255];

    for (uint32 i = 0; i < 255; i++)
    {
        skip[i] = strRefLen;
    }

    for (uint32 i = 0; i < strRefLen; i++)
    {
        skip[(unsigned)strRefData[i]] = strRefLast - (strRefLast - i);
    }

    // Do the reverse search
    const char* searchStart = strData;
    const char* searchPtr = searchStart + endIndex - strRefLen;

    while (searchPtr >= searchStart)
    {
        if (::memcmp(searchPtr, strRefData, strRefLen) == 0)
        {
            return searchPtr - searchStart;
        }

        searchPtr -= skip[(uint8)searchPtr[0]];
    }

    // No match found
    return -1;
}

size_t StringRef::length() const
{
    return len;
}

StringRef StringRef::substring(size_t startIndex) const
{
    return StringRef(strData+startIndex, len-startIndex);
}

StringRef StringRef::substring(size_t startIndex, size_t endIndex) const
{
    return StringRef(strData+startIndex, endIndex - startIndex);
}

bool StringRef::startsWith(const StringRef& strRef) const
{
    if (strRef.len > len)
        return false;

    return ::memcmp(strData, strRef.strData, strRef.len) == 0;
}

bool StringRef::endsWith(const StringRef& strRef) const
{
    if (strRef.len > len)
        return false;

    return ::memcmp(strData + len - strRef.len, strRef.strData, strRef.len) == 0;
}

bool operator<(const StringRef& strRef1, const StringRef& strRef2)
{
    return strRef1.compare(strRef2) < 0;
}

bool operator<(const char* cstr, const StringRef& strRef)
{
    return StringRef(cstr).compare(strRef) < 0;
}

bool operator<(const StringRef& strRef, const char* cstr)
{
    return strRef.compare(StringRef(cstr)) < 0;
}

bool operator<=(const StringRef& strRef1, const StringRef& strRef2)
{
    return strRef1.compare(strRef2) <= 0;
}

bool operator<=(const char* cstr, const StringRef& strRef)
{
    return StringRef(cstr).compare(strRef) <= 0;
}

bool operator<=(const StringRef& strRef, const char* cstr)
{
    return strRef.compare(StringRef(cstr)) <= 0;
}

bool operator>(const StringRef& strRef1, const StringRef& strRef2)
{
    return strRef1.compare(strRef2) > 0;
}

bool operator>(const char* cstr, const StringRef& strRef)
{
    return StringRef(cstr).compare(strRef) > 0;
}

bool operator>(const StringRef& strRef, const char* cstr)
{
    return strRef.compare(StringRef(cstr)) > 0;
}

bool operator>=(const StringRef& strRef1, const StringRef& strRef2)
{
    return strRef1.compare(strRef2) >= 0;
}

bool operator>=(const char* cstr, const StringRef& strRef)
{
    return StringRef(cstr).compare(strRef) >= 0;
}

bool operator>=(const StringRef& strRef, const char* cstr)
{
    return strRef.compare(StringRef(cstr)) >= 0;
}

bool operator==(const StringRef& strRef1, const StringRef& strRef2)
{
    return strRef1.equals(strRef2);
}

bool operator==(const char* cstr, const StringRef& strRef)
{
    return StringRef(cstr).equals(strRef);
}

bool operator==(const StringRef& strRef, const char* cstr)
{
    return strRef.equals(StringRef(cstr));
}

bool operator!=(const StringRef& strRef1, const StringRef& strRef2)
{
    return !strRef1.equals(strRef2);
}

bool operator!=(const char* cstr, const StringRef& strRef)
{
    return !StringRef(cstr).equals(strRef);
}

bool operator!=(const StringRef& strRef, const char* cstr)
{
    return !strRef.equals(StringRef(cstr));
}

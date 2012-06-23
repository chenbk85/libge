#ifndef UNICODEUTIL_H
#define UNICODEUTIL_H

#include <ge/common.h>
#include <ge/text/String.h>

#include <assert.h>

template <class T>
class List;

/*
 * Namespace for Unicode utility functions.
 *
 * Several of these functions are implementations of the samples at from the
 * Unicode Consortium's UTF and BOM FAQ:
 * http://unicode.org/unicode/faq/utf_bom.html#35
 * 
 * UTF-8 bit pattern:
 * <= U+007F   0xxxxxxx
 * <= U+07FF   110xxxxx 10xxxxxx
 * <= U+FFFF   1110xxxx 10xxxxxx 10xxxxxx
 * <= U+1FFFFF 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 */
namespace UnicodeUtil
{
    // Array of UTF-8 lead byte length data from UnicodeUtil.c
    extern const char utf8lenArray[256];

    /*
     * Checks if the passed code point requires surrogate characters if
     * represented in UTF-16.
     */
    static inline
    bool codePointRequiresSurrogates(const utf32 cp)
    {
        return (cp >= 0x10000);
    }

    /*
     * Checks if the passed UTF-16 character is a high surrogate. That is,
     * it checks if is a the first character of a two character sequence.
     */
    static inline
    bool isHighSurrogate(const utf16 ch)
    {
        return (ch & 0xFC00) == 0xD800;
    }

    /*
     * Checks if the passed UTF-16 character is a low surrogate. That is,
     * it checks if is a the second character of a two character sequence.
     */
    static inline
    bool isLowSurrogate(const utf16 ch)
    {
        return (ch & 0xFC00) == 0xDC00;
    }

    /*
     * Returns the code point generated from the passed high and low surrogate
     * pair.
     */
    static inline
    utf32 codePointFromSurrogates(const utf16 high,
                                  const utf16 low)
    {
        return (utf32(high) << 10) + low - 0x35FDC00;
    }

    /*
     * Decomposes a code point into a high and low surrogate pair.
     */
    static inline
    void codePointToSurrogates(const utf32 ch,
                               utf16 pair[2])
    {
        pair[0] = (utf16)(0xD7C0 + (ch >> 10));
        pair[1] = (utf16)(0xDC00 + (ch & 0x3FF));
    }

    /*
     * Checks if the passed code point is a valid code point.
     */
    static inline
    bool isValidCodePoint(const utf32 cp)
    {
        if (cp >= 0xD800 && cp <= 0xDFFF) // Surrogates
        {
            return false;
        }
        return (cp <= 0x10FFFF); // Maximum value
    }

    /*
     * Given the lead character of a UTF-8 code point, this will return the
     * number of bytes used to encode that code point or -1 if the value is
     * an invalid lead byte.
     */
    static inline
    int utf8CodePointLength(const char *c)
    {
        // As there are only 256 possible values, just look it up in an array
        const unsigned char uc = *(const unsigned char*)c;
        return utf8lenArray[uc];
    }

    /*
     * Returns the next code point in the UTF-8 string and moves the passed
     * address to the start of the next code point.
     */
    utf32 nextUtf8CodePoint(const char** buffer);

    /*
     * Returns the next code point in the UTF-16 string and moves the passed
     * address to the start of the next code point.
     */
    utf32 nextUtf16CodePoint(const utf16** buffer);

    /*
     * Converts the passed code point to UTF-8 and places it in the passed
     * buffer. Returns the number of bytes placed, or 0 if the code point is
     * too long to fit.
     */
    uint32 utf8FromCodePoint(const utf32 cp, char* dest);

    /*
     * Returns a pointer to the first non-ASCII character in the buffer, or
     * one byte past the end of the buffer.
     */
    const char* scanAscii(const char* buffer, size_t bufferLen);

    /*
     * Validates that the passed buffer is valid UTF-8.
     */
    bool validateUtf8(const char* buffer, size_t bufferLen);

    /*
     * Unchecked conversion from UTF-8 to UTF-16. Adds result to dest.
     */
    void utf8ToUtf16(const char* buffer, size_t bufferLen, List<utf16>* dest);

    /*
     * Unchecked conversion from UTF-8 to UTF-32. Adds result to dest.
     */
    void utf8ToUtf32(const char* buffer, size_t bufferLen, List<utf32>* dest);

    /*
     * Unchecked conversion from UTF-16 to UTF-8. Adds result to dest.
     */
    void utf16ToUtf8(const utf16* buffer, size_t bufferLen, List<char>* dest);

    /*
     * Unchecked conversion from UTF-16 to UTF-8. Returns result as a String.
     */
    String utf16ToUtf8(const utf16* buffer, size_t bufferLen);

    /*
     * Unchecked conversion from UTF-32 to UTF-8. Adds result to dest.
     */
    void utf32ToUtf8(const utf32* buffer, size_t bufferLen, List<char>* dest);

    /*
     * Unchecked conversion from UTF-32 to UTF-8. Returns result as a String.
     */
    String utf32ToUtf8(const utf32* buffer, size_t bufferLen);

} // End namespace UnicodeUtil

#endif // UNICODEUTIL_H

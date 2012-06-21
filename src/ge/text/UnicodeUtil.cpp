// UnicodeUtil.cpp

#include <ge/data/List.h>
#include <ge/text/UnicodeUtil.h>

#if defined(CHIPSET_X86)

#include <gepriv/X86Info.h>

#if defined(SUPPORTS_SSE2)
const char* scanAscii_sse2(const char* str, size_t len);

const unsigned char* utf8ToUtf16_partial_sse2(const unsigned char* iter,
                                              const unsigned char* end,
                                              List<utf16>* dest);
const unsigned char* utf8ToUtf32_partial_sse2(const unsigned char* iter,
                                              const unsigned char* end,
                                              List<utf32>* dest);
const utf16* utf16ToUtf8_partial_sse2(const utf16* iter,
                                      const utf16* end,
                                      List<char>* dest);
const utf16* utf16ToUtf8_partial_sse2(const utf16* iter,
                                      const utf16* end,
                                      String& ret);
const utf32* utf32ToUtf8_partial_sse2(const utf32* iter,
                                      const utf32* end,
                                      List<char>* dest);
const utf32* utf32ToUtf8_partial_sse2(const utf32* iter,
                                      const utf32* end,
                                      String& ret);
#endif

#if defined(SUPPORTS_AVX)
const char* scanAscii_avx(const char* str, size_t len);
#endif

#endif

#include <cstring>


namespace UnicodeUtil
{

// Length of UTF-8 code point given the starting byte. Values set to -1
// are illegal for starting characters.
// 
// 80 to BF are continuation characters (can't be starting value)
// C0 and C1 could only be used with overlong values
// F5 to FF were valid until UTF-8 was restricted to a max of 4 bytes
const char utf8lenArray[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 00 - 0F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 10 - 1F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 20 - 2F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 30 - 3F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 40 - 4F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 50 - 5F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 60 - 6F
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 70 - 7F
   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 80 - 8F
   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // 90 - 9F
   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // A0 - AF
   -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1, // B0 - BF
   -1,-1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // C0 - CF
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // D0 - DF
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // E0 - EF
    4, 4, 4, 4, 4, 4,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1  // F0 - FF
};

// Returns the code point pointed to by the passed UTF-8 string and moves
// the pointer to the start of the next code point.
utf32 nextUtf8CodePoint(const char** str)
{
    const unsigned char* ustr = (const unsigned char*)*str;
    int len = utf8CodePointLength(*str);

    (*str) += len;

    if (len == 1)
    {
        return *ustr;
    }
    else if (len == 2)
    {
        return ((uint32) (ustr[0] & 0x1F) << 6) |
               ((uint32) (ustr[1] & 0x3F));
    }
    else if (len == 3)
    {
        return ((uint32) (ustr[0] & 0x0F) << 12) |
               ((uint32) (ustr[1] & 0x3F) << 6) |
               ((uint32) (ustr[2] & 0x3F));
    }
    else
    {
        return ((uint32) (ustr[0] & 0x07) << 18) |
               ((uint32) (ustr[1] & 0x3F) << 12) |
               ((uint32) (ustr[2] & 0x3F) << 6) |
               ((uint32) (ustr[3] & 0x3F));
    }
}

// Returns the code point pointed to by the passed UTF-16 string and moves
// the pointer to the start of the next code point.
utf32 nextUtf16CodePoint(const utf16** str)
{
    const utf16* temp = *str;

    if (isHighSurrogate(*temp))
    {
      (*str) += 2;
      return codePointFromSurrogates(temp[0], temp[1]);
    }
    else
    {
        (*str)++;
        return *temp;
    }
}

// Converts the passed code point to UTF-8 and places in the destination
// buffer. Returns the number of bytes added, or 0 on failure.
uint32 utf8FromCodePoint(const utf32 cp, char* dest)
{
    // The scheme here is just brute force masking off of bits and masking on
    // of UTF-8 marker bits. That is (char)(get bits) | (mask bits).
    if (cp < 0x0080)
    {
        dest[0] = (char)cp;
        return 1;
    }
    else if (cp < 0x0800)
    {
        dest[0] = (char)(cp >> 6) | 0xC0;
        dest[1] = (char)(cp & 0x3F) | 0x80;
        return 2;
    }
    else if (cp < 0x10000)
    {
        dest[0] = (char)(cp >> 12) | 0xE0;
        dest[1] = (char)((cp >> 6) & 0x3F) | 0x80;
        dest[2] = (char)(cp & 0x3F) | 0x80;
        return 3;
    }
    else
    {
        dest[0] = (char)(cp >> 18) | 0xF0;
        dest[1] = (char)((cp >> 12) & 0x3F) | 0x80;
        dest[2] = (char)((cp >> 6) & 0x3F) | 0x80;
        dest[3] = (char)(cp & 0x3F) | 0x80;
        return 4;
    }
}

/*
 * Default checkAscii - 253922 ms
 * Vectorized checkAscii (uint32) - 83289 ms
 * Vectorized checkAscii (uint64) - 35381 ms
 * SSE2 checkAscii - 17004 ms
 * AVX checkAscii - 12090 ms
 */
const char* scanAscii(const char* str, size_t len)
{
#if defined(SUPPORTS_AVX)
    if (X86Info::hasAVX())
    {
        return scanAscii_avx(str, len);
    }
#elif defined(SUPPORTS_SSE2)
    if (X86Info::hasSSE2())
    {
        return scanAscii_sse2(str, len);
    }
#endif // Fall through if no vectorized version

    const unsigned char* ustr = (const unsigned char*)str;

    const unsigned char* iter = ustr;
    const unsigned char* end = ustr + len;

    // Check till aligned to 8 byte boundry
    while (iter < end &&
           (*iter) < 128 &&
           ((size_t)iter & 0x7) != 0)
    {
        iter++;
    }

    const unsigned char* fastEnd = (const unsigned char*)((size_t)end & ~((size_t)0x7));

    // Check 8 bytes at a time using 64 bit integers
    while (iter < fastEnd)
    {
        uint64 test = *((size_t*)iter);

        if (test & 0x8080808080808080ULL)
        {
            break;
        }

        iter += 8;
    }

    // Manually test the remainder
    while (iter < end &&
           (*iter) < 128)
    {
        iter++;
    }

    return (const char*)iter;
}

/*
 * This was my best attempt at doing the minimum amount of work for each
 * different length case. It borrows glib's scheme of checking for the
 * overlong form after converting to 32 bit code points.
 *
 * There is some clearly duplicate code in the length 3 and 4 cases. It's
 * hard to eliminate without adding some gotos that seem to slow it down.
 */
bool validateUtf8(const char *buffer, size_t bufferLen)
{
    const unsigned char *start = (const unsigned char*)buffer;
    const unsigned char *end = start + bufferLen;
    const unsigned char *iter = start;
    int32 len;
    uint32 val;

    // Most UTF-8 files are entirely ASCII text, except perhaps for the
    // byte order mark. Try using scanAscii as it's faster.

    // Skip over any byte order mark
    if (bufferLen >= 3 && ::memcmp(buffer, "\xEF\xBB\xBF", 3) == 0)
    {
        iter += 3;
    }

    // Try scanAscii
    iter = (const unsigned char*)scanAscii((char*)iter, end-iter);

    // Loop through UTF-8, validating each code point
    while (iter < end)
    {
        if ((*iter) < 128) // Width of 1
        {
            iter++;
            continue;
        }
        else if (((*iter) & 0xE0) == 0xC0) // Width of 2
        {
            if (iter + 2 >= end)
            {
                break;
            }

            // Check for overlong form (8th or above data bit must be set)
            if ((iter[0] & 0x1E) == 0)
            {
                break;
            }

            // Check continuation byte
            if (((iter[1] & 0xC0) != 0x80))
            {
                break;
            }

            // Don't have to check code point validity. Can't have a large
            // enough value to be one of the invalid ones.
            iter += 2;
            continue;
        }
        else
        {
            // Width of 3 or 4 is long enough we have to check the validity
            // of the code point

            if (((*iter) & 0xF0) == 0xE0) // Width of 3
            {
                if (iter + 3 >= end)
                {
                    break;
                }

                // Check continuation bytes
                if ((iter[1] & 0xC0) != 0x80 ||
                    (iter[2] & 0xC0) != 0x80)
                {
                    break;
                }

                // Convert to code point
                val = ((uint32) (iter[0] & 0x0F) << 12) |
                      ((uint32) (iter[1] & 0x3F) << 6) |
                      ((uint32) (iter[2] & 0x3F));

                // Check for overlong form (11th or above data bit must be set)
                if (val < (1 << 11))
                {
                    break;
                }

                len = 3;
            }
            else if (((*iter) & 0xF8) == 0xF0) // Width of 4
            {
                if (iter + 4 >= end)
                {
                    break;
                }

                // Check continuation bytes
                if ((iter[1] & 0xC0) != 0x80 ||
                    (iter[2] & 0xC0) != 0x80 ||
                    (iter[3] & 0xC0) != 0x80)
                {
                    break;
                }

                // Convert to code point
                val = ((uint32) (iter[0] & 0x07) << 18) |
                      ((uint32) (iter[1] & 0x3F) << 12) |
                      ((uint32) (iter[2] & 0x3F) << 6) |
                      ((uint32) (iter[3] & 0x3F));

                // Check for overlong form (16th or above data bit must be set)
                if (val < (1 << 11))
                {
                    break;
                }

                len = 4;
            }
            else // Invalid length, or not start byte
            {
                break;
            }

            // Check code point legality.
            if (!isValidCodePoint(val))
            {
                break;
            }

            // Increment
            iter += len;
        }
    }

    return (iter - start) == bufferLen;
}

/*
 * For 1000 iterations on 100 MB buffer of all ASCII where dest had adiquate
 * space:
 *
 * SSE2 version: 220 seconds
 * Plain version: 967 seconds
 *
 * Or, about 4.4 times faster on my system.
 */
void utf8ToUtf16(const char* buffer, size_t bufferLen, List<utf16>* dest)
{
    const unsigned char* iter = (const unsigned char*)buffer;
    const unsigned char* end = iter + bufferLen;

#if defined(SUPPORTS_SSE2)
    if (X86Info::hasSSE2())
    {
        iter = utf8ToUtf16_partial_sse2(iter, end, dest);
    }
#endif

    while (iter < end)
    {
        int len = utf8lenArray[*iter];

        if (len == 1)
        {
            dest->addBack(*iter);
        }
        else if (len == 2)
        {
            utf16 c = ((utf16) (iter[0] & 0x1F) << 6) |
                      ((utf16) (iter[1] & 0x3F));
            dest->addBack(c);
        }
        else if (len == 3)
        {
            utf16 c = ((uint32) (iter[0] & 0x0F) << 12) |
                      ((uint32) (iter[1] & 0x3F) << 6) |
                      ((uint32) (iter[2] & 0x3F));
            dest->addBack(c);
        }
        else if (len == 4)
        {
            utf32 cp = ((uint32) (iter[0] & 0x07) << 18) |
                       ((uint32) (iter[1] & 0x3F) << 12) |
                       ((uint32) (iter[2] & 0x3F) << 6) |
                       ((uint32) (iter[3] & 0x3F));

            utf16 surrogatePair[2];
            codePointToSurrogates(cp, surrogatePair);
            dest->addBlockBack(surrogatePair, 2);
        }

        iter += len;
    }
}

/*
 * For 1000 iterations on 100 MB buffer of all ASCII where dest had adiquate
 * space:
 *
 * SSE2 version: 277 seconds
 * Plain version: 871 seconds
 *
 * Or, about 3.1 times faster on my system.
 */
void utf8ToUtf32(const char* buffer, size_t bufferLen, List<utf32>* dest)
{
    const unsigned char* iter = (const unsigned char*)buffer;
    const unsigned char* end = iter + bufferLen;

#if defined(SUPPORTS_SSE2)
    if (X86Info::hasSSE2())
    {
        iter = utf8ToUtf32_partial_sse2(iter, end, dest);
    }
#endif

    while (iter < end)
    {
        int len = utf8lenArray[*iter];
        utf32 cp;

        if (len == 1)
        {
            cp = *iter;
        }
        else if (len == 2)
        {
            cp = ((utf16) (iter[0] & 0x1F) << 6) |
                 ((utf16) (iter[1] & 0x3F));
        }
        else if (len == 3)
        {
            cp = ((uint32) (iter[0] & 0x0F) << 12) |
                 ((uint32) (iter[1] & 0x3F) << 6) |
                 ((uint32) (iter[2] & 0x3F));
        }
        else if (len == 4)
        {
            cp = ((uint32) (iter[0] & 0x07) << 18) |
                 ((uint32) (iter[1] & 0x3F) << 12) |
                 ((uint32) (iter[2] & 0x3F) << 6) |
                 ((uint32) (iter[3] & 0x3F));
        }

        dest->addBack(cp);
        iter += len;
    }
}

/*
 * For 1000 iterations on 200 MB buffer of all ASCII where dest had adiquate
 * space:
 *
 * SSE2 version: 240 seconds
 * Plain version: 702 seconds
 *
 * Or, about 2.9 times faster on my system.
 */
void utf16ToUtf8(const utf16* buffer, size_t bufferLen, List<char>* dest)
{
    const utf16* iter = buffer;
    const utf16* end = buffer + bufferLen;

#if defined(SUPPORTS_SSE2)
    if (X86Info::hasSSE2())
    {
        iter = utf16ToUtf8_partial_sse2(iter, end, dest);
    }
#endif

    while (iter < end)
    {
        char temp[4];
        utf16 high = *iter;

        if (high < 0x80)
        {
            dest->addBack((char)high);
        }
        else if (high < 0x0800)
        {
            temp[0] = (char)(high >> 6) | 0xC0;
            temp[1] = (char)(high & 0x3F) | 0x80;
            dest->addBlockBack(temp, 2);
        }
        else if (!isHighSurrogate(high))
        {
            // Anything else that's not a high surrogate is 3 byte
            temp[0] = (char)(high >> 12) | 0xE0;
            temp[1] = (char)((high >> 6) & 0x3F) | 0x80;
            temp[2] = (char)(high & 0x3F) | 0x80;
            dest->addBlockBack(temp, 3);
        }
        else
        {
            iter++;
            utf16 low = *iter;
            utf32 cp = codePointFromSurrogates(high, low);

            // Will always require 4 bytes for surrogate pair
            temp[0] = (char)(cp >> 18) | 0xF0;
            temp[1] = (char)((cp >> 12) & 0x3F) | 0x80;
            temp[2] = (char)((cp >> 6) & 0x3F) | 0x80;
            temp[3] = (char)(cp & 0x3F) | 0x80;

            dest->addBlockBack(temp, 4);
        }

        iter++;
    }
}

String utf16ToUtf8(const utf16* buffer, size_t bufferLen)
{
    String ret;
    const utf16* iter = buffer;
    const utf16* end = buffer + bufferLen;

#if defined(SUPPORTS_SSE2)
    if (X86Info::hasSSE2())
    {
        iter = utf16ToUtf8_partial_sse2(iter, end, ret);
    }
#endif

    while (iter < end)
    {
        char temp[4];
        utf16 high = *iter;

        if (high < 0x80)
        {
            ret.appendChar((char)high);
        }
        else if (high < 0x0800)
        {
            temp[0] = (char)(high >> 6) | 0xC0;
            temp[1] = (char)(high & 0x3F) | 0x80;
            ret.append(temp, 2);
        }
        else if (!isHighSurrogate(high))
        {
            // Anything else that's not a high surrogate is 3 byte
            temp[0] = (char)(high >> 12) | 0xE0;
            temp[1] = (char)((high >> 6) & 0x3F) | 0x80;
            temp[2] = (char)(high & 0x3F) | 0x80;
            ret.append(temp, 3);
        }
        else
        {
            iter++;
            utf16 low = *iter;
            utf32 cp = codePointFromSurrogates(high, low);

            // Will always require 4 bytes for surrogate pair
            temp[0] = (char)(cp >> 18) | 0xF0;
            temp[1] = (char)((cp >> 12) & 0x3F) | 0x80;
            temp[2] = (char)((cp >> 6) & 0x3F) | 0x80;
            temp[3] = (char)(cp & 0x3F) | 0x80;

            ret.append(temp, 4);
        }

        iter++;
    }

    return ret;
}

/*
 * For 1000 iterations on 200 MB buffer of all ASCII where dest had adiquate
 * space:
 *
 * SSE2 version: 207 seconds
 * Plain version: 694 seconds
 *
 * Or, about 3.3 times faster on my system.
 */
void utf32ToUtf8(const utf32* buffer, size_t bufferLen, List<char>* dest)
{
    const utf32* iter = buffer;
    const utf32* end = buffer + bufferLen;

#if defined(SUPPORTS_SSE2)
    if (X86Info::hasSSE2())
    {
        iter = utf32ToUtf8_partial_sse2(iter, end, dest);
    }
#endif

    while (iter < end)
    {
        char temp[4];
        utf32 cp = *iter;
        int len = utf8FromCodePoint(cp, temp);
        dest->addBlockBack(temp, len);

        iter++;
    }
}

String utf32ToUtf8(const utf32* buffer, size_t bufferLen)
{
    String ret;
    const utf32* iter = buffer;
    const utf32* end = buffer + bufferLen;

#if defined(SUPPORTS_SSE2)
    if (X86Info::hasSSE2())
    {
        iter = utf32ToUtf8_partial_sse2(iter, end, ret);
    }
#endif

    while (iter < end)
    {
        char temp[4];
        utf32 cp = *iter;
        int len = utf8FromCodePoint(cp, temp);
        ret.append(temp, len);

        iter++;
    }

    return ret;
}

} // End namespace UnicodeUtil

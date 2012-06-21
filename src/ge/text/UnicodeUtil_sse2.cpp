// UnicodeUtil_sse2.cpp

/*
 * This file needs to be compliled with support for SSE2 intrinsics.
 */

#include <ge/common.h>

#if defined(CHIPSET_X86)

#include <gepriv/X86Info.h>

#if defined(SUPPORTS_SSE2)

#include <ge/data/List.h>
#include <ge/text/String.h>

#include <emmintrin.h>

/*
 * SSE2 version of scanAscii.
 */
const char* scanAscii_sse2(const char* str, size_t len)
{
    const unsigned char* ustr = (const unsigned char*)str;

    const unsigned char* iter = ustr;
    const unsigned char* end = ustr + len;

    // Manually check until 16 byte aligned.
    while (iter < end &&
           (*iter) < 128 &&
           ((size_t)iter & 0x1F) != 0)
    {
        iter++;
    }

    // Starting from our 16 byte aligned position, calculate the end index for
    // 128 bit operations
    const unsigned char* fastEnd = (const unsigned char*)((size_t)end & ~((size_t)0x1F));

    while (iter < fastEnd)
    {
        // MOVDQA to move bytes to an XMM register
        __m128i test = _mm_load_si128((__m128i*)iter);

        // PMOVMSKB to check high bits. Builds a mask from each of the most
        // significant bits of each byte. We expect all 0s.
        if (_mm_movemask_epi8(test))
        {
            // Break and let manual test find exact byte
            break;
        }

        iter += 16;
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
 * Converts as many ASCII characters as possible to UTF-16 using SSE2
 * instructions. 39175, 39172
 */
const unsigned char* utf8ToUtf16_partial_sse2(const unsigned char* iter,
                                              const unsigned char* end,
                                              List<utf16>* dest)
{
    // Manually check until 16 byte aligned.
    while (iter < end &&
           (*iter) < 128 &&
           ((size_t)iter & 0x1F) != 0)
    {
        dest->addBack(*iter);
        iter++;
    }

    if ((*iter) < 128)
    {
        // Starting from our 16 byte aligned position, calculate the end index for
        // 128 bit operations
        const unsigned char* fastEnd = (const unsigned char*)((size_t)end & ~((size_t)0x1F));

        // Load with zeros
        __m128i zeroMask = _mm_setzero_si128();

        while (iter < fastEnd)
        {
            // MOVDQA to move bytes to an XMM register
            __m128i test = _mm_load_si128((__m128i*)iter);

            // PMOVMSKB to check high bits. Builds a mask from each of the most
            // significant bits of each byte.
            if (_mm_movemask_epi8(test))
            {
                // Break as not pure ASCII
                break;
            }

            ///*
            ALIGN(utf16 fillBlock[16], 16);

            // PUNPCKHBW to interleave 0 bytes with our ASCII to get UTF-16
            __m128i highConv = _mm_unpackhi_epi8(test, zeroMask);

            // MOVQDA to store high part
            _mm_store_si128((__m128i*)fillBlock, highConv);

            // PUNPCKLBW to interleave 0 bytes with our ASCII to get UTF-16
            __m128i lowConv = _mm_unpacklo_epi8(test, zeroMask);

            // MOVQDA to store low part
            _mm_store_si128((__m128i*)&fillBlock[8], lowConv);
            //*/

            /* 1264
            utf16 fillBlock[16];

            fillBlock[0] = iter[0];
            fillBlock[1] = iter[1];
            fillBlock[2] = iter[2];
            fillBlock[3] = iter[3];
            fillBlock[4] = iter[4];
            fillBlock[5] = iter[5];
            fillBlock[6] = iter[6];
            fillBlock[7] = iter[7];
            fillBlock[8] = iter[8];
            fillBlock[9] = iter[9];
            fillBlock[10] = iter[10];
            fillBlock[11] = iter[11];
            fillBlock[12] = iter[12];
            fillBlock[13] = iter[13];
            fillBlock[14] = iter[14];
            fillBlock[15] = iter[15];
            */

            dest->addBlockBack(fillBlock, 16);
            iter += 16;
        }
    }

    return iter;
}

/*
 * Converts as many ASCII characters as possible to UTF-32 using SSE2
 * instructions.
 */
const unsigned char* utf8ToUtf32_partial_sse2(const unsigned char* iter,
                                              const unsigned char* end,
                                              List<utf32>* dest)
{
    // Manually check until 16 byte aligned.
    while (iter < end &&
           (*iter) < 128 &&
           ((size_t)iter & 0x1F) != 0)
    {
        dest->addBack(*iter);
        iter++;
    }

    if ((*iter) < 128)
    {
        // Starting from our 16 byte aligned position, calculate the end index for
        // 128 bit operations
        const unsigned char* fastEnd = (const unsigned char*)((size_t)end & ~((size_t)0x1F));

        while (iter < fastEnd)
        {
            // MOVDQA to move bytes to an XMM register
            __m128i test = _mm_load_si128((__m128i*)iter);

            // PMOVMSKB to check high bits. Builds a mask from each of the most
            // significant bits of each byte.
            if (_mm_movemask_epi8(test))
            {
                // Break as not pure ASCII
                break;
            }

            utf32 fillBlock[16];

            fillBlock[0] = iter[0];
            fillBlock[1] = iter[1];
            fillBlock[2] = iter[2];
            fillBlock[3] = iter[3];
            fillBlock[4] = iter[4];
            fillBlock[5] = iter[5];
            fillBlock[6] = iter[6];
            fillBlock[7] = iter[7];
            fillBlock[8] = iter[8];
            fillBlock[9] = iter[9];
            fillBlock[10] = iter[10];
            fillBlock[11] = iter[11];
            fillBlock[12] = iter[12];
            fillBlock[13] = iter[13];
            fillBlock[14] = iter[14];
            fillBlock[15] = iter[15];

            dest->addBlockBack(fillBlock, 16);
            iter += 16;
        }
    }

    return iter;
}

/*
 * Converts as many UTF-16 characters that are valid ASCII characters as
 * possible to UTF-8 using SSE2 instructions.
 */
const utf16* utf16ToUtf8_partial_sse2(const utf16* iter,
                                      const utf16* end,
                                      List<char>* dest)
{
    // Manually check until 16 byte aligned.
    while (iter < end &&
           (*iter) < 128 &&
           ((size_t)iter & 0x1F) != 0)
    {
        dest->addBack((char)*iter);
        iter++;
    }

    if ((*iter) < 128)
    {
        // Starting from our 16 byte aligned position, calculate the end index for
        // 16 byte aligned operations
        const utf16* fastEnd = (const utf16*)((size_t)end & ~((size_t)0x1F));

        // Load with 8 signed 16 bit values set to -128 (0xFF80 if it was unsigned)
        __m128i andMask = _mm_set1_epi16(-128);

        // Load with zeros
        __m128i zeroMask = _mm_setzero_si128();

        while (iter < fastEnd)
        {
            ALIGN(char fillBlock[16], 16);

            // Move bytes to an XMM register
            __m128i loaded = _mm_load_si128((__m128i*)iter);

            // Locally and with mask
            __m128i andResult = _mm_and_si128(loaded, andMask);

            // Break if anything in andResult is set
            if (_mm_movemask_epi8(andResult) != 0)
            {
                // Break as not pure ASCII
                break;
            }

            // Pack our 16 bit values into 8 bits. Filling remainder with zeros.
            __m128i packed = _mm_packus_epi16(loaded, zeroMask);

            // Store those 8 bit values in fillBlock
            _mm_store_si128((__m128i*)fillBlock, packed);

            // Add the values to dest
            dest->addBlockBack(fillBlock, 8);
            iter += 8;
        }
    }

    return iter;
}

/*
 * Converts as many UTF-16 characters that are valid ASCII characters as
 * possible to UTF-8 using SSE2 instructions.
 */
const utf16* utf16ToUtf8_partial_sse2(const utf16* iter,
                                      const utf16* end,
                                      String& ret)
{
        // Manually check until 16 byte aligned.
    while (iter < end &&
           (*iter) < 128 &&
           ((size_t)iter & 0x1F) != 0)
    {
        ret.appendChar((char)*iter);
        iter++;
    }

    if ((*iter) < 128)
    {
        // Starting from our 16 byte aligned position, calculate the end index for
        // 16 byte aligned operations
        const utf16* fastEnd = (const utf16*)((size_t)end & ~((size_t)0x1F));

        // Load with 8 signed 16 bit values set to -128 (0xFF80 if it was unsigned)
        __m128i andMask = _mm_set1_epi16(-128);

        // Load with zeros
        __m128i zeroMask = _mm_setzero_si128();

        while (iter < fastEnd)
        {
            ALIGN(char fillBlock[16], 16);

            // Move bytes to an XMM register
            __m128i loaded = _mm_load_si128((__m128i*)iter);

            // Locally and with mask
            __m128i andResult = _mm_and_si128(loaded, andMask);

            // Break if anything in andResult is set
            if (_mm_movemask_epi8(andResult) != 0)
            {
                // Break as not pure ASCII
                break;
            }

            // Pack our 16 bit values into 8 bits. Filling remainder with zeros.
            __m128i packed = _mm_packus_epi16(loaded, zeroMask);

            // Store those 8 bit values in fillBlock
            _mm_store_si128((__m128i*)fillBlock, packed);

            // Add the values to dest
            ret.append(fillBlock, 8);
            iter += 8;
        }
    }

    return iter;
}

/*
 * Converts as many UTF-32 characters that are valid ASCII characters as
 * possible to UTF-8 using SSE2 instructions.
 */
const utf32* utf32ToUtf8_partial_sse2(const utf32* iter,
                                      const utf32* end,
                                      List<char>* dest)
{
    // Manually check until 16 byte aligned.
    while (iter < end &&
           (*iter) < 128 &&
           ((size_t)iter & 0x1F) != 0)
    {
        dest->addBack((char)*iter);
        iter++;
    }

    if ((*iter) < 128)
    {
        // Starting from our 16 byte aligned position, calculate the end index for
        // 16 byte aligned operations
        const utf32* fastEnd = (const utf32*)((size_t)end & ~((size_t)0x1F));

        // Load with 8 signed 16 bit values set to -128 (0xFFFFFF80 if it was unsigned)
        __m128i andMask = _mm_set1_epi32(-128);

        // Load with zeros
        __m128i zeroMask = _mm_setzero_si128();

        while (iter < fastEnd)
        {
            ALIGN(char fillBlock[16], 16);

            // Move bytes to an XMM register
            __m128i loaded = _mm_load_si128((__m128i*)iter);

            // Locally and with mask
            __m128i andResult = _mm_and_si128(loaded, andMask);

            // Break if anything in andResult is set
            if (_mm_movemask_epi8(andResult) != 0)
            {
                // Break as not pure ASCII
                break;
            }

            // Pack our 32 bit values into 16 bits, filling remainder with zeros.
            __m128i packed = _mm_packs_epi32(loaded, zeroMask);

            // Then pack those 16 bit values into 8 bits, filling remainder with zeros.
            packed = _mm_packs_epi16(packed, zeroMask);

            // Store those 8 bit values in fillBlock
            _mm_store_si128((__m128i*)fillBlock, packed);

            // Add the values to dest
            dest->addBlockBack(fillBlock, 4);
            iter += 4;
        }
    }

    return iter;
}

/*
 * Converts as many UTF-32 characters that are valid ASCII characters as
 * possible to UTF-8 using SSE2 instructions.
 */
const utf32* utf32ToUtf8_partial_sse2(const utf32* iter,
                                      const utf32* end,
                                      String& ret)
{
        // Manually check until 16 byte aligned.
    while (iter < end &&
           (*iter) < 128 &&
           ((size_t)iter & 0x1F) != 0)
    {
        ret.appendChar((char)*iter);
        iter++;
    }

    if ((*iter) < 128)
    {
        // Starting from our 16 byte aligned position, calculate the end index for
        // 16 byte aligned operations
        const utf32* fastEnd = (const utf32*)((size_t)end & ~((size_t)0x1F));

        // Load with 8 signed 16 bit values set to -128 (0xFFFFFF80 if it was unsigned)
        __m128i andMask = _mm_set1_epi32(-128);

        // Load with zeros
        __m128i zeroMask = _mm_setzero_si128();

        while (iter < fastEnd)
        {
            ALIGN(char fillBlock[16], 16);

            // Move bytes to an XMM register
            __m128i loaded = _mm_load_si128((__m128i*)iter);

            // Locally and with mask
            __m128i andResult = _mm_and_si128(loaded, andMask);

            // Break if anything in andResult is set
            if (_mm_movemask_epi8(andResult) != 0)
            {
                // Break as not pure ASCII
                break;
            }

            // Pack our 32 bit values into 16 bits, filling remainder with zeros.
            __m128i packed = _mm_packs_epi32(loaded, zeroMask);

            // Then pack those 16 bit values into 8 bits, filling remainder with zeros.
            packed = _mm_packs_epi16(packed, zeroMask);

            // Store those 8 bit values in fillBlock
            _mm_store_si128((__m128i*)fillBlock, packed);

            // Add the values to dest
            ret.append(fillBlock, 4);
            iter += 4;
        }
    }

    return iter;
}

#endif // SUPPORTS_SSE2

#endif // CHIPSSET_X86

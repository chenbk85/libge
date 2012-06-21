// UnicodeUtil_avx.cpp

/*
 * This file needs to be compliled with support for Intel AVX intrinsics.
 */

#include <ge/common.h>

#if defined(CHIPSET_X86)

#include <gepriv/X86Info.h>

#if defined(SUPPORTS_AVX)

#include <ge/data/List.h>
#include <ge/text/String.h>

#include <immintrin.h>

/*
 * AVX lacks pack/unpack operations until AVX2, so we really can't do much
 * besides scanAscii.
 */

/*
 * AVX version of scanAscii.
 */
const char* scanAscii_avx(const char* str, size_t len)
{
    const unsigned char* ustr = (const unsigned char*)str;

    const unsigned char* iter = ustr;
    const unsigned char* end = ustr + len;

    // Manually check until 32 byte aligned.
    while (iter < end &&
           (*iter) < 128 &&
           ((size_t)iter & 0x3F) != 0)
    {
        iter++;
    }

    // Starting from our 32 byte aligned position, calculate the end index for
    // 32 byte operations
    const unsigned char* fastEnd = (const unsigned char*)((size_t)end & ~((size_t)0x3F));

    // Build a mask where all 32 bytes have the high bit set
    __m256i mask = _mm256_set1_epi8(-128);

    while (iter < fastEnd)
    {
        // VMOVDQA to move bytes to an YMM register
        __m256i test = _mm256_load_si256((__m256i*)iter);

        // VPTEST to and against mask to check if high bits are set. VPTEST
        // checks the ZF flag which is set if the result of the masking is
        // all 0s.
        if (!_mm256_testz_si256(test, mask))
        {
            // Break and let manual test find exact byte
            break;
        }

        iter += 32;
    }

    // Manually test the remainder
    while (iter < end &&
           (*iter) < 128)
    {
        iter++;
    }

    return (const char*)iter;
}

#endif // SUPPORTS_AVX

#endif // CHIPSET_X86

// X86Info.h

#ifndef X86INFO_H
#define X86INFO_H

#include <ge/text/StringRef.h>

// Always support the different intrinsics
#define SUPPORTS_LZCNT
#define SUPPORTS_POPCNT
#define SUPPORTS_PCLMUL
#define SUPPORTS_SSE
#define SUPPORTS_SSE2
#define SUPPORTS_SSE3
#define SUPPORTS_SSSE3
#define SUPPORTS_SSE4_1
#define SUPPORTS_SSE4_2
#define SUPPORTS_SSE4A

// AVX and AES require Visual Sutdio 10 SP1 or later
#if (_MSC_FULL_VER >= 160040219)
#define SUPPORTS_AVX
#define SUPPORTS_AES
#endif

/*
 * Class that wraps the information extractable by the X86 CPUID instruction.
 * Where the compiler has a macro indicating the capability, the macro will
 * be used instead of consulting the CPU.
 */
namespace X86Info
{
    // Globals pulled in from X86Info.cpp
    // Should be accessed through accessor functions.
    EXPORT extern bool x86info_hasCMPXCHG16B;
    EXPORT extern bool x86info_hasLZCNT;
    EXPORT extern bool x86info_hasPOPCNT;
    EXPORT extern bool x86info_hasPCLMULQDQ;
    EXPORT extern bool x86info_hasSSE;
    EXPORT extern bool x86info_hasSSE2;
    EXPORT extern bool x86info_hasSSE3;
    EXPORT extern bool x86info_hasSupplementalSSE3;
    EXPORT extern bool x86info_hasSSE4_1;
    EXPORT extern bool x86info_hasSSE4_2;
    EXPORT extern bool x86info_hasSSE4a;
    EXPORT extern bool x86info_hasAVX;
    EXPORT extern bool x86info_hasAES;

    /*
     * Initializes the X86Info namespace globals
     */
    void initializeX86Info();

    static inline
    bool hasLZCNT()
    {
        return x86info_hasLZCNT;
    }

    static inline
    bool hasPOPCNT()
    {
        return x86info_hasPOPCNT;
    }

    static inline
    bool hasPCLMULQDQ()
    {
        return x86info_hasPCLMULQDQ;
    }

    static inline
    bool hasSSE()
    {
#if defined(_M_X64) || \
    (defined(_MSC_VER) && defined(_M_IX86_FP) && (_M_IX86_FP >= 1))
        return true;
#else
        return x86info_hasSSE;
#endif
    }

    static inline
    bool hasSSE2()
    {
#if defined(_M_X64) || \
    (defined(_MSC_VER) && defined(_M_IX86_FP) && (_M_IX86_FP >= 2))
        return true;
#else
        return x86info_hasSSE2;
#endif
    }

    static inline
    bool hasSSE3()
    {
        return x86info_hasSSE3;
    }

    static inline
    bool hasSupplementalSSE3()
    {
        return x86info_hasSupplementalSSE3;
    }

    static inline
    bool hasSSE4_1()
    {
        return x86info_hasSSE4_1;
    }

    static inline
    bool hasSSE4_2()
    {
        return x86info_hasSSE4_2;
    }

    static inline
    bool hasSSE4a()
    {
        return x86info_hasSSE4a;
    }

    static inline
    bool hasAVX()
    {
        // _M_IX86_FP does not indicate AVX in Visual Studio 2010
        return x86info_hasAVX;
    }

    static inline
    bool hasAES()
    {
        return x86info_hasAES;
    }
}

#endif // X86INFO_H

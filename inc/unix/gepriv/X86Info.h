// X86Info.h

#ifndef X86INFO_H
#define X86INFO_H

#include <ge/text/StringRef.h>

// As GCC will spit out compile errors if we have unsupported intrinics,
// create macro guards

#if defined(__LZCNT__)
#define SUPPORTS_LZCNT
#endif
#if defined(__POPCNT__)
#define SUPPORTS_POPCNT
#endif
#if defined(__PCLMUL__)
#define SUPPORTS_PCLMUL
#endif
#if defined(__SSE__)
#define SUPPORTS_SSE
#endif
#if defined(__SSE2__)
#define SUPPORTS_SSE2
#endif
#if defined(__SSE3__)
#define SUPPORTS_SSE3
#endif
#if defined(__SSSE3__)
#define SUPPORTS_SSSE3
#endif
#if defined(__SSE4_1__)
#define SUPPORTS_SSE4_1
#endif
#if defined(__SSE4_2__)
#define SUPPORTS_SSE4_2
#endif
#if defined(__SSE4A__)
#define SUPPORTS_SSE4A
#endif
#if defined(__AVX__)
#define SUPPORTS_AVX
#endif
#if defined(__AES__)
#define SUPPORTS_AES
#endif

/*
 * Class that wraps the information extractable by the X86 CPUID instruction.
 * Where the compiler has a macro indicating the capability, the macro will
 * be used instead of consulting the CPU.
 */
namespace X86Info
{
    static inline
    bool hasLZCNT()
    {
#if defined(__LZCNT__)
        return true;
#else
        return false;
#endif
    }

    static inline
    bool hasPOPCNT()
    {
#if defined(__POPCNT__)
        return true;
#else
        return false;
#endif
    }

    static inline
    bool hasPCLMULQDQ()
    {
#if defined(__PCLMUL__)
        return true;
#else
        return false;
#endif
    }

    static inline
    bool hasSSE()
    {
#if defined(__SSE__)
        return true;
#else
        return false;
#endif
    }

    static inline
    bool hasSSE2()
    {
#if defined(__SSE2__)
        return true;
#else
        return false;
#endif
    }

    static inline
    bool hasSSE3()
    {
#if defined(__SSE3__)
        return true;
#else
        return false;
#endif
    }

    static inline
    bool hasSupplementalSSE3()
    {
#if defined(__SSSE3__)
        return true;
#else
        return false;
#endif
    }

    static inline
    bool hasSSE4_1()
    {
#if defined(__SSE4_1__)
        return true;
#else
        return false;
#endif
    }

    static inline
    bool hasSSE4_2()
    {
#if defined(__SSE4_2__)
        return true;
#else
        return false;
#endif
    }

    static inline
    bool hasSSE4a()
    {
#if defined(__SSE4A__)
        return true;
#else
        return false;
#endif
    }

    static inline
    bool hasAVX()
    {
#if defined(__AVX__)
        return true;
#else
        return false;
#endif
    }

    static inline
    bool hasAES()
    {
#if defined(__AES__)
        return true;
#else
        return false;
#endif
    }
}

#endif // X86INFO_H

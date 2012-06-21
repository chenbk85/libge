// UInt64.h

#ifndef UINT64_H
#define UINT64_H

#include <ge/common.h>
#include <ge/text/String.h>
#include <ge/text/StringRef.h>

// Maximum size
#ifndef UINT64_MAX
#define UINT64_MAX 18446744073709551615ULL
#endif

// Maximum length as a string in base 10
#define UINT64_STRMAX 20

namespace UInt64
{
    EXPORT
    String uint64ToString(uint64 value, uint32 radix=10);

    EXPORT
    uint32 uint64ToBuffer(char* buffer, uint32 bufferLen, uint64 value, uint32 radix=10);

    EXPORT
    uint64 parseUInt64(StringRef strRef, bool* ok=NULL, uint32 radix=10);

    EXPORT
    uint32 uint64CountOnes(uint64 value);

    EXPORT
    uint32 uint64CountZeros(uint64 value);

    EXPORT
    bool uint64Parity(uint64 value);

    EXPORT
    bool uint64IsPow2(uint64 value);

    EXPORT
    uint64 uint64NextPow2(uint64 value);

    EXPORT
    uint64 uint64RotateLeft(uint64 value, uint32 shift);

    EXPORT
    uint64 uint64RotateRight(uint64 value, uint32 shift);
}

#endif // UINT64_H

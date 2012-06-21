// UInt8.h

#ifndef UINT8_H
#define UINT8_H

#include <ge/common.h>
#include <ge/text/String.h>
#include <ge/text/StringRef.h>

// Maximum size
#ifndef UINT8_MAX
#define UINT8_MAX 255
#endif

// Maximum length as a string in base 10
#define UINT8_STRMAX 3

namespace UInt8
{
    EXPORT
    String uint8ToString(uint8 value, uint32 radix=10);

    EXPORT
    uint32 uint8ToBuffer(char* buffer, uint32 bufferLen, uint8 value, uint32 radix=10);

    EXPORT
    uint8 parseUInt8(StringRef strRef, bool* ok=NULL, uint32 radix=10);

    EXPORT
    uint32 uint8CountOnes(uint8 value);

    EXPORT
    uint32 uint8CountZeros(uint8 value);

    EXPORT
    bool uint8Parity(uint8 value);

    EXPORT
    bool uint8IsPow2(uint8 value);

    EXPORT
    uint8 uint8NextPow2(uint8 value);

    EXPORT
    uint8 uint8RotateLeft(uint8 value, uint8 shift);

    EXPORT
    uint8 uint8RotateRight(uint8 value, uint8 shift);
}

#endif // UINT8_H

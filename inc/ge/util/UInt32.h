// UInt32.h

#ifndef UINT32_H
#define UINT32_H

#include <ge/common.h>
#include <ge/text/String.h>
#include <ge/text/StringRef.h>

// Maximum size
#ifndef UINT32_MAX
#define UINT32_MAX 4294967295U
#endif

// Maximum length as a string in base 10
#define UINT32_STRMAX 10

namespace UInt32
{
    String uint32ToString(uint32 value, uint32 radix=10);

    uint32 uint32ToBuffer(char* buffer, uint32 bufferLen, uint32 value, uint32 radix=10);

    uint32 parseUInt32(StringRef strRef, bool* ok=NULL, uint32 radix=10);

    uint32 uint32CountOnes(uint32 value);

    uint32 uint32CountZeros(uint32 value);

    bool uint32Parity(uint32 value);

    bool uint32IsPow2(uint32 value);

    uint32 uint32NextPow2(uint32 value);

    uint32 uint32RotateLeft(uint32 value, uint32 amount);

    uint32 uint32RotateRight(uint32 value, uint32 amount);
}

#endif // UINT32_H

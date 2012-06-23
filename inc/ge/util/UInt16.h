// UInt16.h

#ifndef UINT16_H
#define UINT16_H

#include <ge/common.h>
#include <ge/text/String.h>
#include <ge/text/StringRef.h>

// Maximum size
#ifndef UINT16_MAX
#define UINT16_MAX 65535
#endif

// Maximum length as a string in base 10
#define UINT16_STRMAX 5

namespace UInt16
{
    String uint16ToString(uint16 value, uint32 radix=10);

    uint32 uint16ToBuffer(char* buffer, uint32 bufferLen, uint16 value, uint32 radix=10);

    uint16 parseUInt16(StringRef strRef, bool* ok=NULL, uint32 radix=10);

    uint32 uint16CountOnes(uint16 value);

    uint32 uint16CountZeros(uint16 value);

    bool uint16Parity(uint16 value);

    bool uint16IsPow2(uint16 value);

    uint16 uint16NextPow2(uint16 value);

    uint16 uint16RotateLeft(uint16 value, uint8 shift);

    uint16 uint16RotateRight(uint16 value, uint8 shift);
}

#endif // UINT16_H

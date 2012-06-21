// Int32.h

#ifndef INT32_H
#define INT32_H

#include <ge/common.h>
#include <ge/text/String.h>
#include <ge/text/StringRef.h>

// Maximum and minimum sizes
#ifndef INT32_MAX
#define INT32_MAX 2147483647
#endif
#ifndef INT32_MIN
#define INT32_MIN (-INT32_MAX - 1)
#endif

// Maximum length as a string in base 10
#define INT32_STRMAX 11

namespace Int32
{
    EXPORT
    String int32ToString(int32 value, uint32 radix=10);

    EXPORT
    uint32 int32ToBuffer(char* buffer, uint32 bufferLen, int32 value, uint32 radix=10);

    EXPORT
    int32 parseInt32(StringRef strRef, bool* ok=NULL, uint32 radix=10);
}

#endif // INT32_H

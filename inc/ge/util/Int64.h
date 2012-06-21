// Int64.h

#ifndef INT64_H
#define INT64_H

#include <ge/common.h>
#include <ge/text/String.h>
#include <ge/text/StringRef.h>

// Maximum and minimum sizes
#ifndef INT64_MAX
#define INT64_MAX 9223372036854775807LL
#endif
#ifndef INT64_MIN
#define INT64_MIN (-INT64_MAX - 1LL)
#endif

// Maximum length as a string in base 10
#define INT64_STRMAX 20

namespace Int64
{
    EXPORT
    String int64ToString(int64 value, uint32 radix=10);

    EXPORT
    uint32 int64ToBuffer(char* buffer, uint32 bufferLen, int64 value, uint32 radix=10);

    EXPORT
    int64 parseInt64(StringRef strRef, bool* ok, uint32 radix);
}

#endif // INT32_H

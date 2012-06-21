// Int8.h

#ifndef INT8_H
#define INT8_H

#include <ge/common.h>
#include <ge/text/String.h>
#include <ge/text/StringRef.h>

// Maximum and minimum sizes
#ifndef INT8_MAX
#define INT8_MAX 127
#endif
#ifndef INT8_MIN
#define INT8_MIN (-128)
#endif

// Maximum length as a string in base 10
#define INT8_STRMAX 4

namespace Int8
{
    EXPORT
    String int8ToString(int8 value, uint32 radix=10);

    EXPORT
    uint32 int8ToBuffer(char* buffer, uint32 bufferLen, int8 value, uint32 radix=10);

    EXPORT
    int8 parseInt8(StringRef strRef, bool* ok=NULL, uint32 radix=10);
}

#endif // INT8_H

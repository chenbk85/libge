// Int16.h

#ifndef INT16_H
#define INT16_H

#include <ge/common.h>
#include <ge/text/String.h>
#include <ge/text/StringRef.h>

// Maximum and minimum sizes
#ifndef INT16_MAX
#define INT16_MAX 32767
#endif
#ifndef INT16_MIN
#define INT16_MIN (-32768)
#endif

// Maximum length as a string in base 10
#define INT16_STRMAX 6

namespace Int16
{
    EXPORT
    String int16ToString(int16 value, uint32 radix=10);

    EXPORT
    uint32 int16ToBuffer(char* buffer, uint32 bufferLen, int16 value, uint32 radix=10);

    EXPORT
    int16 parseInt16(StringRef strRef, bool* ok=NULL, uint32 radix=10);
}

#endif // INT16_H

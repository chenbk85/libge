// Float.h

#ifndef FLOAT_H
#define FLOAT_H

#include <ge/common.h>
#include <ge/text/String.h>
#include <ge/text/StringRef.h>

/*
 * Collection of functions operating on floats.
 */
namespace Float
{
    String floatToString(float value,
                         char format='f',
                         int32 precision=-1);

    uint32 floatToBuffer(char* buffer,
                         uint32 bufferLen,
                         float value,
                         char format='f',
                         int32 precision=-1);

    float parseFloat(StringRef strRef,
                     bool* ok=NULL);

    String localeFloatToString(float value,
                               char format='f',
                               int32 precision=-1);

    uint32 localeFloatToBuffer(char* buffer,
                               uint32 bufferLen,
                               float value,
                               char format='f',
                               int32 precision=-1);

    float parseLocaleFloat(StringRef strRef,
                           bool* ok=NULL);
}

#endif // FLOAT_H

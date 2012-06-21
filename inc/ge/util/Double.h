// Double.h

#ifndef DOUBLE_H
#define DOUBLE_H

#include <ge/common.h>
#include <ge/text/String.h>
#include <ge/text/StringRef.h>

namespace Double
{
    EXPORT
    String doubleToString(double value,
                          char format='f',
                          int32 precision=-1);

    EXPORT
    uint32 doubleToBuffer(char* buffer,
                          uint32 bufferLen,
                          double value,
                          char format='f',
                          int32 precision=-1);

    EXPORT
    double parseDouble(StringRef strRef,
                       bool* ok=NULL);

    EXPORT
    String localeDoubleToString(double value,
                                char format='f',
                                int32 precision=-1);

    EXPORT
    uint32 localeDoubleToBuffer(char* buffer,
                                uint32 bufferLen,
                                double value,
                                char format='f',
                                int32 precision=-1);

    EXPORT
    double parseLocaleDouble(StringRef strRef,
                             bool* ok=NULL);
}

#endif // DOUBLE_H

// Double.h

#ifndef DOUBLE_H
#define DOUBLE_H

#include <ge/common.h>
#include <ge/text/String.h>
#include <ge/text/StringRef.h>

namespace Double
{
    String doubleToString(double value,
                          char format='f',
                          int32 precision=-1);

    uint32 doubleToBuffer(char* buffer,
                          uint32 bufferLen,
                          double value,
                          char format='f',
                          int32 precision=-1);

    double parseDouble(StringRef strRef,
                       bool* ok=NULL);

    String localeDoubleToString(double value,
                                char format='f',
                                int32 precision=-1);

    uint32 localeDoubleToBuffer(char* buffer,
                                uint32 bufferLen,
                                double value,
                                char format='f',
                                int32 precision=-1);

    double parseLocaleDouble(StringRef strRef,
                             bool* ok=NULL);
}

#endif // DOUBLE_H

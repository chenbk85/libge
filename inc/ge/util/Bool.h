// Bool.h

#ifndef BOOL_H
#define BOOL_H

#include <ge/common.h>
#include <ge/text/StringRef.h>

namespace Bool
{
    EXPORT
    String boolToString(bool value);

    EXPORT
    uint32 int32ToBuffer(char* buffer, uint32 bufferLen, bool value);

    EXPORT
    bool parseBool(StringRef strRef, bool* ok=NULL);

    EXPORT
    void setBool(bool* target, bool value);
}

#endif // BOOL_H

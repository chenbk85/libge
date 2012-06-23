// Bool.h

#ifndef BOOL_H
#define BOOL_H

#include <ge/common.h>
#include <ge/text/StringRef.h>

namespace Bool
{
    String boolToString(bool value);

    uint32 int32ToBuffer(char* buffer, uint32 bufferLen, bool value);

    bool parseBool(StringRef strRef, bool* ok=NULL);

    void setBool(bool* target, bool value);
}

#endif // BOOL_H

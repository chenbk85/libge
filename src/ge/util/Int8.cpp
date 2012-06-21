// Int8.cpp

#include <ge/util/Int8.h>

#include <ge/util/Bool.h>
#include <ge/util/Int32.h>
#include <ge/util/UInt8.h>

String Int8::int8ToString(int8 value, uint32 radix)
{
    return Int32::int32ToString((int32)value, radix);
}

uint32 Int8::int8ToBuffer(char* buffer, uint32 bufferLen, int8 value, uint32 radix)
{
    return Int32::int32ToBuffer(buffer, (int32)value, radix);
}

int8 Int8::parseInt8(StringRef strRef, bool* ok, uint32 radix)
{
    int32 ret = Int32::parseInt32(strRef, ok, radix);

    if (ret < INT8_MIN || ret > INT8_MAX)
    {
        Bool::setBool(ok, false);
        return 0;
    }

    return (int8)ret;
}

// Int16.cpp

#include <ge/util/Int16.h>

#include <ge/util/Bool.h>
#include <ge/util/Int32.h>

String Int16::int16ToString(int16 value, uint32 radix)
{
    return Int32::int32ToString((int32)value, radix);
}

uint32 Int16::int16ToBuffer(char* buffer, uint32 bufferLen, int16 value, uint32 radix)
{
    return Int32::int32ToBuffer(buffer, (int32)value, radix);
}

int16 Int16::parseInt16(StringRef strRef, bool* ok, uint32 radix)
{
    int32 ret = Int32::parseInt32(strRef, ok, radix);

    if (ret < INT16_MIN || ret > INT16_MAX)
    {
        Bool::setBool(ok, false);
        return 0;
    }

    return (int16)ret;
}

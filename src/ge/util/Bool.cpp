// Bool.cpp

#include <ge/util/Bool.h>

bool Bool::parseBool(StringRef strRef, bool* ok)
{
    uint32 len = strRef.length();

    if (len == 4)
    {
        bool ret = strRef.engEqualsIgnoreCase("true");
        setBool(ok, ret);
        return ret;
    }
    else if (len == 5)
    {
        bool ret = strRef.engEqualsIgnoreCase("false");
        setBool(ok, ret);
        return ret;
    }
    else
    {
        setBool(ok, false);
        return false;
    }
}

void Bool::setBool(bool* target, bool value)
{
    if (target != NULL)
    {
        (*target) = value;
    }
}

// INetUtil.h

#ifndef INET_UTIL_H
#define INET_UTIL_H

#include <ge/text/StringRef.h>

namespace INetUtil
{
    /*
     * Converts an IPv4 address string to binary. Assumes dest is at least 4
     * bytes.
     *
     * Returns true on successful parse, false on failure.
     */
    EXPORT
    bool ipv4AddrStringToBinary(StringRef src, uint8* dest);

    /*
     * Converts an IPv6 address string to binary. Assumes dest is at least 16
     * bytes.
     *
     * Returns true on successful parse, false on failure.
     */
    EXPORT
    bool ipv6AddrStringToBinary(StringRef src, uint8* dest);
};

#endif // INET_UTIL_H

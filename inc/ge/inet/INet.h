// INet.h

#ifndef INET_H
#define INET_H

#include <ge/text/String.h>

// Generic enum for "IPv4 or IPv6" distinctions
enum INetProt_Enum
{
    INET_PROT_UNKNOWN,
    INET_PROT_IPV4,
    INET_PROT_IPV6
};

// Select interest flag bits
#define INTEREST_READ 1
#define INTEREST_WRITE 2
#define INTEREST_ERROR 4

namespace INet
{
    String getHostName();
};

#endif // INET_H

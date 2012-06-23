// INetAddress.cpp

#include <ge/inet/INetAddress.h>

#include <ge/data/ShortList.h>
#include <ge/inet/INetUtil.h>
#include <ge/util/Bool.h>

#include <cstdio>
#include <cstring>
#include <Ws2tcpip.h>

INetAddress::INetAddress() :
    m_family(INET_PROT_UNKNOWN)
{
    ::memset(m_addr, 0, sizeof(m_addr));
}

INetAddress::~INetAddress()
{

}

INetAddress::INetAddress(const INetAddress& other)
{
    m_family = other.m_family;
    ::memcpy(m_addr, other.m_addr, sizeof(m_addr));
}

INetAddress& INetAddress::operator=(const INetAddress& other)
{
    if (this != &other)
    {
        m_family = other.m_family;
        ::memcpy(m_addr, other.m_addr, sizeof(m_addr));
    }

    return *this;
}

#if defined(HAVE_RVALUE)
INetAddress::INetAddress(INetAddress&& other)
{
    m_family = other.m_family;
    ::memcpy(m_addr, other.m_addr, sizeof(m_addr));
}

INetAddress& INetAddress::operator=(INetAddress&& other)
{
    m_family = other.m_family;
    ::memcpy(m_addr, other.m_addr, sizeof(m_addr));
    return *this;
}
#endif

INetProt_Enum INetAddress::getFamily() const
{
    return m_family;
}

const unsigned char* INetAddress::getAddrData() const
{
    return m_addr;
}

String INetAddress::toString() const
{
    char buffer[46]; // Documented maximum

    if (m_family == INET_PROT_IPV4)
    {
        in_addr ipv4Address;
        ::memcpy(&ipv4Address, m_addr, 4);
        ::inet_ntop(AF_INET, &ipv4Address, buffer, sizeof(buffer));
    }
    else if (m_family == INET_PROT_IPV6)
    {
        in6_addr ipv6Address;
        ::memcpy(&ipv6Address, m_addr, 16);
        ::inet_ntop(AF_INET6, &ipv6Address, buffer, sizeof(buffer));
    }

    return String(buffer);
}

// Static member functions --------------------------------------------------

INetAddress INetAddress::getAddrAny(INetProt_Enum family)
{
    INetAddress ret;

    // The bind any address is 0s for both protocols so just fill with
    // zeros regardless
    ret.m_family = family;
    ::memset(ret.m_addr, 0, 16);

    return ret;
}

INetAddress INetAddress::getLoopback(INetProt_Enum family)
{
    static const char rawIpv4[4] = {127,0,0,1};
    static const char rawIpv6[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};

    INetAddress ret;
    ret.m_family = family;

    if (family == INET_PROT_IPV4)
    {
        ::memcpy(ret.m_addr, rawIpv4, 4);
        ::memset(ret.m_addr+4, 0, 12);
    }
    else if (family == INET_PROT_IPV6)
    {
        ::memcpy(ret.m_addr, rawIpv6, 16);
    }

    return ret;
}

INetAddress INetAddress::fromString(StringRef strRef, bool* valid)
{
    INetAddress ret;

    if (INetUtil::ipv6AddrStringToBinary(strRef, ret.m_addr))
    {
        ret.m_family = INET_PROT_IPV6;
        Bool::setBool(valid, true);
    }
    else if (INetUtil::ipv4AddrStringToBinary(strRef, ret.m_addr))
    {
        ret.m_family = INET_PROT_IPV4;
        Bool::setBool(valid, true);
    }
    else
    {
        Bool::setBool(valid, false);
    }

    return ret;
}

INetAddress INetAddress::fromBytes(INetProt_Enum family, unsigned char* rawBytes)
{
    INetAddress ret;

    if (family == INET_PROT_IPV4)
    {
        ret.m_family = INET_PROT_IPV4;
        ::memcpy(ret.m_addr, rawBytes, 4);
        ::memset(ret.m_addr+4, 0, 12);
    }
    else if (family == INET_PROT_IPV6)
    {
        ret.m_family = INET_PROT_IPV6;
        ::memcpy(ret.m_addr, rawBytes, 16);
    }

    return ret;
}

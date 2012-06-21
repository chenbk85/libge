// INetAddress.h

#ifndef INET_ADDRESS_H
#define INET_ADDRESS_H

#include <ge/inet/INet.h>
#include <ge/text/String.h>
#include <ge/text/StringRef.h>

/*
 * Represents an IPv4 or IPv6 address.
 */
class INetAddress
{
    friend class Socket;
public:
    INetAddress();
    ~INetAddress();

    INetAddress(const INetAddress& other);
    INetAddress& operator=(const INetAddress& other);

#if defined(HAVE_RVALUE)
    INetAddress(INetAddress&& other);
    INetAddress& operator=(INetAddress&& other);
#endif

    String toString() const;

    INetProt_Enum getFamily() const;
    const unsigned char* getAddrData() const;

    static INetAddress fromBytes(INetProt_Enum family, unsigned char* rawBytes);
    static INetAddress fromString(StringRef strRef, bool* valid=NULL);

private:
    INetProt_Enum m_family;
    unsigned char m_addr[16];
};

#endif // INET4_ADDRESS_H

// INet.cpp

#include <ge/inet/INet.h>

#include <ge/io/IOException.h>
#include <ge/text/UnicodeUtil.h>

#include <gepriv/WinUtil.h>

#include <winsock2.h>

namespace INet
{

String getHostName()
{
    char buffer[256];

    int res = ::gethostname(buffer, sizeof(buffer));

    if (res == SOCKET_ERROR)
    {
        throw IOException(WinUtil::getError(::WSAGetLastError(),
            "gethostname",
            "INet::getHostName"));
    }

    // TODO: It seems that non-ASCII host names are illegal, but there
    // is nothing blocking it. Not sure how to handle.

    return String(buffer);
}

} // End namespace INet

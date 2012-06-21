// INet.cpp

#include <ge/inet/INet.h>

#include <ge/data/ShortList.h>
#include <ge/inet/INetException.h>
#include <gepriv/UnixUtil.h>

#include <cerrno>
#include <unistd.h>

namespace INet
{

String getHostName()
{
    ShortList<char, 256> listBuf(256);

    int res = ::gethostname(listBuf.data(), listBuf.size());

    while (res == -1 &&
           errno == ENAMETOOLONG)
    {
        listBuf.uninitializedResize(listBuf.size()*2);

        res = ::gethostname(listBuf.data(), listBuf.size());
    }

    if (res == -1)
    {
        throw INetException(UnixUtil::getLastErrorMessage());
    }

    return String(listBuf.data());
}


} // End namespace INet

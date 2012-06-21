// Selector.h

#ifndef SELECTOR_H
#define SELECTOR_H

#include <ge/data/List.h>
#include <ge/inet/Socket.h>
#include <ge/inet/SelectKey.h>

/*
 * Logical wrapper for level triggered IO polling on sockets. Wraps some
 * fundamental set of OS functionality such as select, epoll, etc. This
 * class is not thread safe.
 *
 * Sockets are added to the set of file handles being polled with addSocket
 * and removed with removeSocket. The select function gives you a set of
 * Socket objects in the form of SelectKeys, which have a pointer to the
 * Socket and a mask indicating which IO types are ready.
 *
 * The Selector does not "own" the Sockets. If the destructor of a Socket
 * is called while still in the Selector's set of Sockets, you will get a
 * resource leak and undefined behavior.
 */
class Selector
{
public:
    Selector();
    ~Selector();

    void addSocket(Socket& socket, uint32 interestMask);
    bool removeSocket(Socket& socket);

    void select(List<SelectKey>* readyKeys);
    bool select(List<SelectKey>* readyKeys, uint32 timeout);

private:
//#if (NTDDI_VERSION < NTDDI_VISTA)
    List<Socket*> m_socketList;
    List<uint32> m_interestList;
//#else
//    List<WSAPOLLFD> m_pollFd;
//#endif
};

#endif // SELECTOR_H

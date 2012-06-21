// Selector.h

#ifndef SELECTOR_H
#define SELECTOR_H

#include <ge/common.h>

#include <ge/data/List.h>

class Selector
{
public:
    Selector();
    ~Selector();

    void select();
    void select(uint32 timeoutMilli);

private:
    Selector(const Selector& other) DELETED;
    Selector& operator=(const Selector& other) DELETED;

#if defined(__linux__)

#else
    //List<pollfd> m_pollFds;
#endif
};

#endif // SELECTOR_H

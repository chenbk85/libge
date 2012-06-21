// SelectKey.h

#ifndef SELECT_KEY_H
#define SELECT_KEY_H

#include <ge/inet/Socket.h>

/*
 * A SelectKey is a logical representation of an entry in a Selector.
 *
 * You add Sockets and a mask of the desired events you are interested in to
 * the Selector. When you call select, it gives you a list of SelectKeys
 * representing Sockets ready for IO. The interestMask indicates what IO is
 * available.
 */
class SelectKey
{
public:
    Socket* socket;
    int interestMask;
};

#endif // SELECT_KEY_H

// ShortList.h

#ifndef SHORT_LIST_H
#define SHORT_LIST_H

#include <ge/data/List.h>

/*
 * ShortList is a derived class of List which has identical behavior except
 * that it has a block of non-dynamically allocated memory in which to store
 * elements, until this space is filled.
 *
 * Functions that generally only handle small numbers of elements can use
 * ShortList to avoid allocation in the general case. It similarly allows
 * calling of functions that expect a List reference without any memory
 * allocation.
 *
 * The number of non-dynamically allocated elements is determined by the
 * second template parameter. The ShortList class is quite small, but each
 * unique pair of template parameters will cause some additional code
 * generation. It is thus generally advisiable to reuse template parameters
 * by sticking to powers of 2, or some other simple rule.
 */
template<typename T, int ShortCount>
class ShortList : public List<T>
{
public:
    ShortList();
    explicit ShortList(size_t initialSize);
    ShortList(const ShortList& other);
    ShortList(const List<T>& other);
    ~ShortList();

private:
    size_t getMinReserved() OVERRIDE;
    T* makeNewBuffer(size_t newSize) OVERRIDE;
    void cleanupBuffer(T* start, T* end) OVERRIDE;

    // char type to avoid default construction and destruction
    // Blindly aligning to 16 bytes due to compiler issues, but it
    // should be enough for anything.
    ALIGN(char m_shortData[ShortCount*sizeof(T)], 16);
};

template<typename T, int ShortCount>
ShortList<T, ShortCount>::ShortList() : List<T>()
{
    List<T>::_start = (T*)m_shortData;
    List<T>::_iter = List<T>::_start;
    List<T>::_end = List<T>::_start + ShortCount;
}

template<typename T, int ShortCount>
ShortList<T, ShortCount>::ShortList(size_t initialSize) : List<T>()
{
    if (initialSize <= ShortCount)
    {
        List<T>::_start = (T*)m_shortData;
        List<T>::_end = List<T>::_start + ShortCount;
    }
    else
    {
        List<T>::_start = List<T>::makeNewBuffer(initialSize);
        List<T>::_end = List<T>::_start + initialSize;
    }

    List<T>::_iter = List<T>::_start;
}

template<typename T, int ShortCount>
ShortList<T, ShortCount>::ShortList(const ShortList& other) : List<T>()
{
    if (other.size() <= ShortCount)
    {
        List<T>::_start = (T*)m_shortData;
        List<T>::_end = List<T>::_start + ShortCount;
    }
    else
    {
        uint64 otherSize = other.size();
        List<T>::_start = CppUtil::uninitializedAlloc<T>(otherSize);
        List<T>::_end = List<T>::_start + otherSize;
    }

    try
    {
        std::uninitialized_copy(other._start, other._end, List<T>::_start);
    }
    catch (...)
    {
        cleanupBuffer(List<T>::_start, List<T>::_start);
        throw;
    }

    List<T>::_iter = List<T>::_start;
}

template<typename T, int ShortCount>
ShortList<T, ShortCount>::ShortList(const List<T>& other) : List<T>()
{
    if (other.size() <= ShortCount)
    {
        List<T>::_start = (T*)m_shortData;
        List<T>::_end = List<T>::_start + ShortCount;
    }
    else
    {
        uint64 otherSize = other.size();
        List<T>::_start = CppUtil::uninitializedAlloc<T>(otherSize);
        List<T>::_end = List<T>::_start + otherSize;
    }

    try
    {
        std::uninitialized_copy(other._start, other._end, List<T>::_start);
    }
    catch (...)
    {
        cleanupBuffer(List<T>::_start, List<T>::_start);
        throw;
    }

    List<T>::_iter = List<T>::_start;
}

template<typename T, int ShortCount>
ShortList<T, ShortCount>::~ShortList()
{
    cleanupBuffer(List<T>::_start, List<T>::_iter);

    // Defend against base class cleanup
    List<T>::_start = NULL;
    List<T>::_end = NULL;
}

template<typename T, int ShortCount>
size_t ShortList<T, ShortCount>::getMinReserved()
{
    return ShortCount;
}

template<typename T, int ShortCount>
T* ShortList<T, ShortCount>::makeNewBuffer(size_t bufferSize)
{
    if (bufferSize < ShortCount)
        return (T*)m_shortData;

    return List<T>::makeNewBuffer(bufferSize);
}

template<typename T, int ShortCount>
void ShortList<T, ShortCount>::cleanupBuffer(T* start, T* end)
{
    CppUtil::destroy(start, end);

    if (start != (T*)m_shortData)
    {
        CppUtil::uninitializedFree(start);
    }
}

#endif // SHORT_LIST_H

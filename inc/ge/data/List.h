// List.h

#ifndef LIST_H
#define LIST_H

#include <ge/common.h>
#include <ge/util/CppUtil.h>

#include <cassert>
#include <algorithm>
#include <cstddef>
#include <vector>

#define LIST_MIN_SIZE (size_t)8

/*
 * List is a wrapper around a dynamically allocated array.
 *
 * It is designed to allow for simple implementation of SmallList
 * (see SmallList.h).
 */
template<typename T>
class List
{
public:
    class ConstIterator
    {
        friend class List;
    public:
        ConstIterator();
        ConstIterator(const List* ownerList, T* ptr);
        ConstIterator(const ConstIterator& other);
        ConstIterator& operator=(const ConstIterator& other);

        bool hasPrevious() const;
        const T& prev();

        bool hasNext() const;
        const T& next();

        const T& value();

    protected:
        const List* _owner;
        T* _ptr;
    };

    class Iterator : public ConstIterator
    {
    public:
        Iterator();
        Iterator(const List* ownerList, T* ptr);
        Iterator(const Iterator& other);
        Iterator& operator=(const Iterator& other);

        T& prev();
        T& next();

        void insert(const T& value);
        void insert(T&& value);

        void remove();

        void set(const T& value);
        void set(T&& value);

        T& value();
    };

    List();
    explicit List(size_t initialReserved);
    virtual ~List();

    List(const List& other);
    List& operator=(const List& other);

    List(List&& other);
    List& operator=(List&& other);

    void clear();

    T* data();

    T& get(size_t index);
    const T& get(size_t index) const;

    T& front();
    const T& front() const;

    T& back();
    const T& back() const;

    void insert(size_t index, const T& value);
    void insert(size_t index, T&& value);

    void remove(size_t index);

    void set(size_t index, const T& value);
    void set(size_t index, T&& value);

    void addBack(const T& value);
    void addBack(T&& value);
    void addBlockBack(const T* values, size_t count);

    void popBack();

    size_t size() const;

    size_t reserved() const;

    bool isEmpty() const;

    void reserve(size_t reserveSize);
    void resize(size_t newSize, T fillValue=T());
    void uninitializedResize(size_t newSize);

    ConstIterator iterator(size_t startIndex=0) const;
    Iterator iterator(size_t startIndex=0);

protected:
    virtual size_t getMinReserved();
    virtual T* makeNewBuffer(size_t bufferSize);
    virtual void cleanupBuffer(T* start, T* end);

    T* _start;
    T* _end;
    T* _iter;
};

// Implementation of List ---------------------------------------------------

template<typename T>
List<T>::List()
{
    _start = NULL;
    _iter = NULL;
    _end = NULL;
}

template<typename T>
List<T>::List(size_t initialReserved)
{
    _start = makeNewBuffer(initialReserved);
    _iter = _start;
    _end = _start + initialReserved;
}

template<typename T>
List<T>::List(const List& other)
{
    size_t reserved = other.size();

    _start = makeNewBuffer(reserved);

    try
    {
        std::uninitialized_copy(other._start, other._iter, _start);
    }
    catch (...)
    {
        cleanupBuffer(_start, 0);
        throw;
    }

    _iter = _start;
    _end = _start + reserved;
}

template<typename T>
List<T>::~List()
{
    cleanupBuffer(_start, _iter);
}

template<typename T>
List<T>& List<T>::operator=(const List<T>& other)
{
    size_t otherReserved = other.reserved();

    if (reserved() < other.reserved())
    {
        cleanupBuffer(_start, _iter);

        _start = NULL;
        _start = makeNewBuffer(otherReserved);
        _end = _start + otherReserved;
    }
    else
    {
        CppUtil::destroy(_start, _iter);
    }

    try
    {
        std::uninitialized_copy(other._start, other._iter, _start);
    }
    catch (...)
    {
        cleanupBuffer(_start, 0);
        _start = NULL;
        _end = 0;
        _iter = 0;
        throw;
    }

    _iter = _start + other.size();

    return *this;
}

template<typename T>
List<T>::List(List&& other)
{
    _start = other._start;
    _end = other._end;
    _iter = other._iter;

    other._start = NULL;
    other._end = 0;
    other._iter = 0;
}

template<typename T>
List<T>& List<T>::operator=(List<T>&& other)
{
    if (this != &other)
    {
        _start = other._start;
        _end = other._end;
        _iter = other._iter;

        other._start = NULL;
        other._end = NULL;
        other._iter = NULL;
    }

    return *this;
}

template<typename T>
void List<T>::clear()
{
    CppUtil::destroy(_start, _iter);
    _start = NULL;
    _end = NULL;
    _iter = NULL;
}

template<typename T>
T* List<T>::data()
{
    return _start;
}

template<typename T>
T& List<T>::get(size_t index)
{
    assert(size() > index);

    return _start[index];
}

template<typename T>
const T& List<T>::get(size_t index) const
{
    assert(size() > index);

    return _start[index];
}

template<typename T>
T& List<T>::front()
{
    assert(size() > 0);

    return _start[0];
}

template<typename T>
const T& List<T>::front() const
{
    assert(size() > 0);

    return _start[0];
}

template<typename T>
T& List<T>::back()
{
    assert(size() > 0);

    return *(_iter - 1);
}

template<typename T>
const T& List<T>::back() const
{
    assert(size() > 0);

    return *(_iter - 1);
}

template<typename T>
void List<T>::insert(size_t index, const T& value)
{
    // We're limited in the exception guarantee we can provide for insert
    // unless we allocate a new array every time. Moving elements to the right
    // creates the possibility of an exception in copy or assignment which
    // can't be cleaned up safely as moving the data back may also raise an
    // exception.
    //
    // The current implementation rotates the value in, which at least
    // guarantees no duplicate objects when something throws.
    addBack(value);
    std::rotate(_start + index, _iter - 1, _iter);
}

template<typename T>
void List<T>::insert(size_t index, T&& value)
{
    addBack(std::forward(value));
    std::rotate(_start + index, _iter - 1, _iter);
}

template<typename T>
void List<T>::remove(size_t index)
{
    std::rotate(_start + index, _start + index + 1, _iter);
    CppUtil::destroy(_iter);
}

template<typename T>
void List<T>::set(size_t index, const T& value)
{
    assert(size() != 0 && index < size());

    _start[index] = value;
}

template<typename T>
void List<T>::set(size_t index, T&& value)
{
    assert(size() != 0 && index < size());

    _start[index] = std::forward(value);
}

template<typename T>
void List<T>::addBack(const T& value)
{
    if (_iter == _end)
    {
        size_t oldReserved = reserved();
        size_t newReserved = std::max(LIST_MIN_SIZE, oldReserved * 2);
        reserve(newReserved);
    }

    CppUtil::copyConstruct(_iter, value);
    _iter++;
}

template<typename T>
void List<T>::addBack(T&& value)
{
    if (_iter == _end)
    {
        size_t oldReserved = reserved();
        size_t newReserved = std::max(LIST_MIN_SIZE, oldReserved * 2);
        reserve(newReserved);
    }

    CppUtil::moveConstruct(_iter, std::forward<T>(value));
    _iter++;
}

template<typename T>
void List<T>::addBlockBack(const T* values, size_t count)
{
    size_t oldSize = size();
    size_t oldReserved = reserved();

    if (oldReserved - oldSize < count)
    {
        size_t newReserved = std::max(LIST_MIN_SIZE, oldReserved * 2);
        newReserved = std::max(newReserved, oldSize + count);
        reserve(newReserved);
    }

    std::uninitialized_copy(values, values+count, _iter);
    _iter += count;
}

template<typename T>
void List<T>::popBack()
{
    assert(size() > 0);

    _iter--;
    CppUtil::destroy(_iter);
}

template<typename T>
size_t List<T>::size() const
{
    return _iter - _start;
}

template<typename T>
size_t List<T>::reserved() const
{
    return _end - _start;
}

template<typename T>
bool List<T>::isEmpty() const
{
    return _start == _iter;
}

template<typename T>
void List<T>::reserve(size_t reserveSize)
{
    size_t oldReserved = reserved();

    if (reserveSize <= oldReserved)
        return;

    T* newData = makeNewBuffer(reserveSize);

    try
    {
        std::uninitialized_copy(_start, _iter, newData);
    }
    catch (...)
    {
        cleanupBuffer(newData, 0);
        throw;
    }

    size_t oldSize = size();

    cleanupBuffer(_start, _iter);
    _start = newData;
    _end = _start + reserveSize;
    _iter = _start + oldSize;
}

template<typename T>
void List<T>::resize(size_t newSize, T fillValue)
{
    size_t oldSize = size();

    if (newSize < oldSize)
    {
        CppUtil::destroy(_start + newSize, _iter);
        _iter = _start + newSize;
    }
    else
    {
        size_t oldReserved = reserved();

        if (newSize > oldReserved)
        {
            reserve(newSize);
        }

        T* cleanIter = _iter;

        try
        {
            while (cleanIter != _end)
            {
                CppUtil::copyConstruct(cleanIter, fillValue);
                cleanIter++;
            }
        }
        catch (...)
        {
            CppUtil::destroy(_iter+1, cleanIter);
            throw;
        }

        _iter = _start + newSize;
    }
}

template<typename T>
void List<T>::uninitializedResize(size_t newSize)
{
    reserve(newSize);
    _iter = _end;
}

template<typename T>
typename List<T>::ConstIterator List<T>::iterator(size_t startIndex) const
{
    return ConstIterator(this, _start + startIndex);
}

template<typename T>
typename List<T>::Iterator List<T>::iterator(size_t startIndex)
{
    return Iterator(this, _start + startIndex);
}

/*
 * getMinReserved is virtual and overridden by ShortList to return the
 * second template parameter of ShortList, indicating the non-dynamically
 * allocated memory available.
 */
template<typename T>
size_t List<T>::getMinReserved()
{
    return 0;
}

/*
 * makeNewBuffer is virtual and overridden by ShortList to allow returning
 * the "short" non-dynamically allocated buffer.
 */
template<typename T>
T* List<T>::makeNewBuffer(size_t bufferSize)
{
    // Returning NULL on zero size is important for the default constructor
    // and clear
    if (bufferSize == 0)
        return NULL;

    return CppUtil::uninitializedAlloc<T>(bufferSize);
}

/*
 * cleanupBuffer is virtual and overridden by ShortList to allow for cleanup
 * of the "short" non-dynamically allocated buffer.
 */
template<typename T>
void List<T>::cleanupBuffer(T* start, T* end)
{
    CppUtil::destroy(start, end);
    CppUtil::uninitializedFree(start);
}

// ConstIterator functions --------------------------------------------------

template<typename T>
List<T>::ConstIterator::ConstIterator() :
    _owner(NULL),
    _ptr(NULL)
{
}

template<typename T>
List<T>::ConstIterator::ConstIterator(const List* ownerList, T* ptr) :
    _owner(ownerList),
    _ptr(ptr)
{
}

template<typename T>
List<T>::ConstIterator::ConstIterator(const ConstIterator& other) :
    _owner(other._owner),
    _ptr(other._ptr)
{
}

template<typename T>
typename List<T>::ConstIterator&
    List<T>::ConstIterator::operator=(const ConstIterator& other)
{
    _owner = other._owner;
    _ptr = other._ptr;
    return *this;
}

template<typename T>
bool List<T>::ConstIterator::hasPrevious() const
{
    return _ptr != _owner->_start;
}

template<typename T>
const T& List<T>::ConstIterator::prev()
{
    T& ret = *_ptr;
    _ptr--;
    return ret;
}

template<typename T>
bool List<T>::ConstIterator::hasNext() const
{
    return _ptr != _owner->_iter;
}

template<typename T>
const T& List<T>::ConstIterator::next()
{
    T& ret = *_ptr;
    _ptr++;
    return ret;
}

template<typename T>
const T& List<T>::ConstIterator::value()
{
    return *_ptr;
}

// Iterator functions -------------------------------------------------------

template<typename T>
List<T>::Iterator::Iterator() :
    ConstIterator(NULL, NULL)
{
}

template<typename T>
List<T>::Iterator::Iterator(const List* ownerList, T* ptr) :
    ConstIterator(ownerList, ptr)
{
}

template<typename T>
List<T>::Iterator::Iterator(const Iterator& other) :
    ConstIterator(other._owner, other._ptr)
{
}

template<typename T>
typename List<T>::Iterator&
    List<T>::Iterator::operator=(const Iterator& other)
{
    this->_owner = other._owner;
    this->_ptr = other._ptr;
    return *this;
}

template<typename T>
T& List<T>::Iterator::prev()
{
    return (T&)ConstIterator::prev();
}

template<typename T>
T& List<T>::Iterator::next()
{
    return (T&)ConstIterator::next();
}

template<typename T>
void List<T>::Iterator::insert(const T& value)
{
    this->_owner->insert(this->_ptr - this->_owner->_start, value);
}

template<typename T>
void List<T>::Iterator::insert(T&& value)
{
    this->_owner->insert(this->_ptr - this->_owner->_start, std::forward(value));
}

template<typename T>
void List<T>::Iterator::remove()
{
    this->_owner->remove(this->_ptr - this->_owner->_start);
}

template<typename T>
void List<T>::Iterator::set(const T& value)
{
    this->_owner->set(this->_ptr - this->_owner->_start, value);
}

template<typename T>
void List<T>::Iterator::set(T&& value)
{
    this->_owner->set(this->_ptr - this->_owner->_start, std::forward(value));
}

template<typename T>
T& List<T>::Iterator::value()
{
    return (T&)ConstIterator::value();
}

#undef LIST_MIN_SIZE

#endif // LIST_H

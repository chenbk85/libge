// Deque.h

#ifndef DEQUE_H
#define DEQUE_H

#include <ge/common.h>
#include <ge/util/CppUtil.h>

/*
 * Double ended queue implemented as a circular list.
 *
 * TODO: Insert/remove logic
 */
template<typename T>
class Deque
{
public:
    class ConstIterator
    {
        friend class Deque;
    public:
        ConstIterator();
        ConstIterator(const Deque* owner, T* ptr);

        ConstIterator(const ConstIterator& other);
        ConstIterator& operator=(const ConstIterator& other);

        bool hasPrev() const;
        const T& prev();

        bool hasNext() const;
        const T& next();

        const T& value();

    protected:
        const Deque* _owner;
        T* _ptr;
    };

    class Iterator : public ConstIterator
    {
        friend class Deque;
    public:
        Iterator();
        Iterator(const Deque* owner, T* ptr);

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


    Deque();
    explicit Deque(size_t initialReserved);
    ~Deque();

    Deque(const Deque& other);
    Deque& operator=(const Deque& other);

    Deque(Deque&& other);
    Deque& operator=(Deque&& other);

    void addFront(const T& value);
    void addFront(T&& value);

    void addBack(const T& value);
    void addBack(T&& value);

    T& front();
    const T& front() const;

    T& back();
    const T& back() const;

    void popFront();
    void popBack();

    void insert(size_t index, const T& value);
    void insert(size_t index, T&& value);

    void remove(size_t index);

    T& get(size_t index);
    const T& get(size_t index) const;

    void set(size_t index, const T& value);
    void set(size_t index, T&& value);

    size_t size() const;
    size_t reserved() const;

    void reserve(size_t size);

    Iterator iterator(size_t startIndex=0);
    ConstIterator iterator(size_t startIndex=0) const;

private:
    T* next(T*) const;
    T* prev(T*) const;
    T* indexPtr(size_t index) const;

    T* _data;
    T* _head;
    T* _tail;
    T* _end;
    size_t _size;
};

template<typename T>
Deque<T>::Deque() :
    _data(NULL),
    _head(NULL),
    _tail(NULL),
    _end(NULL),
    _size(0)
{
}

template<typename T>
Deque<T>::Deque(size_t initialReserved) :
    _size(0),
    _reserved(initialReserved)
{
    _data = CppUtil::uninitializedAlloc(initialReserved);
    _head = _data;
    _tail = _data;
    _end = _data + initialReserved;
}

template<typename T>
Deque<T>::~Deque()
{
    if (_size != 0)
    {
        while (_tail != _head)
        {
            CppUtil::destroy(_tail);
            _tail = next(_tail);
        }
    }

    CppUtil::uninitializedFree(_data);
}

template<typename T>
Deque<T>::Deque(const Deque& other)
{
    _reserved = other.reserved();

    _data = CppUtil::uninitializedAlloc(_reserved);
    _tail = _data;
    _head = _data;

    T* iter = other._tail;

    while (iter != other._head)
    {
        (*_head) = *iter;

        iter = other.next(iter);
        _head++;
    }
}

template<typename T>
Deque<T>& Deque<T>::operator=(const Deque& other)
{
    if (&other != this)
    {
        size_t otherReserved = other.reserved();

        // Allocate buffer to copy into
        T* newData = CppUtil::uninitializedAlloc(otherReserved);

        // Copy objects, reverting on exception
        T* newIter;

        try
        {
            newIter = newData;
            T* otherIter = other._tail;

            while (otherIter != other._head)
            {
                (*newIter) = *otherIter;

                otherIter = other.next(otherIter);
                newIter++;
            }
        }
        catch (...)
        {
            CppUtil::destroy(newData, newIter+1);
            CppUtil::uninitializedFree(newData);
            throw;
        }

        CppUtil::uninitializedFree(_data);

        _data = newData;
        _end = newData + otherReserved;
        _tail = _data;
        _head = _data + other._size - 1;
        _size = other._size;
    }

    return *this;
}

template<typename T>
Deque<T>::Deque(Deque&& other)
{
    if (&other == this)
        return;

    _data = other._data;
    _head = other._head;
    _tail = other._tail;
    _size = other._size;
    _reserved = other._reserved;

    other._data = NULL;
    other._head = NULL;
    other._tail = NULL;
    other._size = 0;
    other._reserved = 0;
}

template<typename T>
Deque<T>& Deque<T>::operator=(Deque&& other)
{
    if (&other != this)
    {
        CppUtil::uninitializedFree(_data);

        _data = other._data;
        _head = other._head;
        _tail = other._tail;
        _size = other._size;
        _reserved = other._reserved;

        other._data = NULL;
        other._head = NULL;
        other._tail = NULL;
        other._size = 0;
        other._reserved = 0;
    }

    return *this;
}

template<typename T>
void Deque<T>::addFront(const T& value)
{
    if (_size == reserved())
        reserve(_size * 2);

    _tail = prev(_tail);
    CppUtil::copyConstruct(_tail, value);
    _size++;
}

template<typename T>
void Deque<T>::addFront(T&& value)
{
    if (_size == reserved())
        reserve(_size * 2);

    _tail = prev(_tail);
    CppUtil::moveConstruct(_tail, std::forward<T>(value));
    _size++;
}

template<typename T>
void Deque<T>::addBack(const T& value)
{
    if (_size == reserved())
        reserve(_size * 2);

    CppUtil::copyConstruct(_head, value);
    _head = next(_head);
    _size++;
}

template<typename T>
void Deque<T>::addBack(T&& value)
{
    if (_size == reserved())
        reserve(_size * 2);

    CppUtil::moveConstruct(_head, std::forward<T>(value));
    _head = next(_head);
    _size++;
}

template<typename T>
T& Deque<T>::front()
{
    return *_tail;
}

template<typename T>
const T& Deque<T>::front() const
{
    return *_tail;
}

template<typename T>
T& Deque<T>::back()
{
    return *prev(_head);
}

template<typename T>
const T& Deque<T>::back() const
{
    return *prev(_head);
}

template<typename T>
void Deque<T>::popFront()
{
    CppUtil::destroy(_tail);
    _tail = next(_tail);
}

template<typename T>
void Deque<T>::popBack()
{
    _head = prev(_head);
    CppUtil::destroy(_head);
}

template<typename T>
void insert(size_t index, const T& value)
{
    // TODO
}

template<typename T>
void insert(size_t index, T&& value)
{
    // TODO
}

template<typename T>
void remove(size_t index)
{
    // TODO
}

template<typename T>
T& Deque<T>::get(size_t index)
{
    return *indexPtr(index);
}

template<typename T>
const T& Deque<T>::get(size_t index) const
{
    return *indexPtr(index);
}

template<typename T>
void Deque<T>::set(size_t index, const T& value)
{
    T* valPtr = indexPtr(index);
    (*valPtr) = value;
}

template<typename T>
void Deque<T>::set(size_t index, T&& value)
{
    T* valPtr = indexPtr(index);
    (*valPtr) = std::forward(value);
}

template<typename T>
size_t Deque<T>::size() const
{
    return _size;
}

template<typename T>
size_t Deque<T>::reserved() const
{
    return (_end - _data);
}

template<typename T>
void Deque<T>::reserve(size_t reserveSize)
{
    if (reserveSize < reserved())
        return;

    if (reserveSize < 16)
        reserveSize = 16; 

    T* newData = CppUtil::uninitializedAlloc<T>(reserveSize);

    if (_size != 0)
    {
        T* srcIter = _tail;
        T* destIter = newData;

        for (size_t i = 0; i < _size; i++)
        {
            CppUtil::moveConstruct(destIter, std::move(*srcIter));

            srcIter = next(srcIter);
            destIter++;
        }

        CppUtil::uninitializedFree(_data);
    }

    _data = newData;
    _head = newData + _size;
    _tail = newData;
    _end = newData + reserveSize;
}

template<typename T>
typename Deque<T>::Iterator
    Deque<T>::iterator(size_t startIndex=0)
{
    return Iterator(this, indexPtr(startIndex));
}

template<typename T>
typename Deque<T>::ConstIterator
    Deque<T>::iterator(size_t startIndex=0) const
{
    return ConstIterator(this, indexPtr(startIndex));
}

template<typename T>
T* Deque<T>::next(T* ptr) const
{
    ptr++;

    if (ptr == _end)
        ptr = _data;

    return ptr;
}

template<typename T>
T* Deque<T>::prev(T* ptr) const
{
    if (ptr == _data)
        return _end - 1;

    return ptr - 1;
}

template<typename T>
T* Deque<T>::indexPtr(size_t index) const
{
    T* ret = _tail + index;

    if (ret < _end)
        return ret;

    return ret - (_end - _data);
}

// ConstIterator ------------------------------------------------------------

template<typename T>
Deque<T>::ConstIterator::ConstIterator() :
    _owner(NULL),
    _ptr(NULL)
{
}

template<typename T>
Deque<T>::ConstIterator::ConstIterator(const Deque* owner, T* ptr) :
    _owner(owner),
    _ptr(ptr)
{
}

template<typename T>
Deque<T>::ConstIterator::ConstIterator(const ConstIterator& other) :
    _owner(other._owner),
    _ptr(other._ptr)
{
}

template<typename T>
typename Deque<T>::ConstIterator&
    Deque<T>::ConstIterator::operator=(const ConstIterator& other)
{
    _owner = other._owner;
    _ptr = other._ptr;
    return *this;
}

template<typename T>
bool Deque<T>::ConstIterator::hasPrev() const
{
    return (_ptr != _owner->prev(_owner->_tail));
}

template<typename T>
const T& Deque<T>::ConstIterator::prev()
{
    T* ret = _ptr;
    _ptr = _owner->prev(_ptr);
    return *ret;
}

template<typename T>
bool Deque<T>::ConstIterator::hasNext() const
{
    return (_ptr != _owner->_head);
}

template<typename T>
const T& Deque<T>::ConstIterator::next()
{
    T* ret = _ptr;
    _ptr = _owner->next(_ptr);
    return *ret;
}

template<typename T>
const T& Deque<T>::ConstIterator::value()
{
    return *_ptr;
}

// Iterator functions -------------------------------------------------------

template<typename T>
Deque<T>::Iterator::Iterator() :
    _owner(NULL),
    _ptr(NULL)
{
}

template<typename T>
Deque<T>::Iterator::Iterator(const Deque* owner, T* ptr) :
    ConstIterator(owner, ptr)
{
}

template<typename T>
Deque<T>::Iterator::Iterator(const Iterator& other) :
    ConstIterator(other._owner, other._ptr)
{
}

template<typename T>
typename Deque<T>::Iterator&
    Deque<T>::Iterator::operator=(const Iterator& other)
{
    _owner = other._owner;
    _ptr = other._ptr;
    return *this;
}

template<typename T>
T& Deque<T>::Iterator::prev()
{
    return (T&)ConstIterator::prev();
}

template<typename T>
T& Deque<T>::Iterator::next()
{
    return (T&)ConstIterator::next();
}

template<typename T>
void Deque<T>::Iterator::remove()
{
    // TODO
}

template<typename T>
void Deque<T>::Iterator::insert(const T& value)
{
    // TODO
}

template<typename T>
void Deque<T>::Iterator::insert(T&& value)
{
    // TODO
}

template<typename T>
void Deque<T>::Iterator::set(const T& value)
{
    // TODO
}

template<typename T>
void Deque<T>::Iterator::set(T&& value)
{
    // TODO
}

template<typename T>
T& Deque<T>::Iterator::value()
{
    return (T&)ConstIterator::value();
}

#endif // DEQUE_H

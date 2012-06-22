// DLinkedList.h

#ifndef DLINKED_LIST_H
#define DLINKED_LIST_H

#include <ge/common.h>

#include <cassert>
#include <utility>

/*
 * Doubly linked list.
 */
template <typename T>
class DLinkedList
{
private:
    class Node;

public:
    class ConstIterator
    {
        friend class DLinkedList;

    public:
        ConstIterator();

        ConstIterator(const ConstIterator& other);
        ConstIterator& operator=(const ConstIterator& other);

        bool hasPrev() const;
        const T& prev();

        bool hasNext() const;
        const T& next();

        const T& value();

    protected:
        ConstIterator(Node* node);

        Node* m_node;
    };

    class Iterator : public ConstIterator
    {
        friend class DLinkedList;

    public:
        Iterator();

        Iterator(const Iterator& other);
        Iterator& operator=(const Iterator& other);

        T& prev();
        T& next();

        T& value();

        void insert(const T& value);
        void insert(T&& value);

        void remove();

        void set(const T& value);
        void set(T&& value);

    private:
        Iterator(Node* node);
    };

    DLinkedList();
    ~DLinkedList();
    
    DLinkedList(const DLinkedList& other);
    DLinkedList& operator=(const DLinkedList& other);

    DLinkedList(DLinkedList&& other);
    DLinkedList& operator=(DLinkedList&& other);

    T& front();
    const T& front() const;

    T& back();
    const T& back() const;

    void addFront(const T& value);
    void addBack(const T& value);

    void addFront(T&& value);
    void addBack(T&& value);

    void popFront();
    void popBack();

    size_t size() const;

    ConstIterator iterator() const;
    Iterator iterator();

private:
    class Node
    {
    public:
        Node() :
            m_data(T()),
            m_prev(NULL),
            m_next(NULL)
        {
        }

        Node(const T& value) :
            m_data(value),
            m_prev(NULL),
            m_next(NULL)
        {
        }

        Node(const T&& value) :
            m_data(std::forward(value)),
            m_prev(NULL),
            m_next(NULL)
        {
        }

        T m_data;
        Node* m_prev;
        Node* m_next;
    };

    Node* m_head;
    Node* m_tail;
    size_t m_size;
};

template <typename T>
DLinkedList<T>::DLinkedList() :
    m_head(NULL),
    m_tail(NULL),
    m_size(0)
{
}

template <typename T>
DLinkedList<T>::~DLinkedList()
{
    Node* delIter = m_head;

    while (delIter)
    {
        Node* temp = delIter;
        delIter = delIter->m_next;
        delete temp;
    }
}

template <typename T>
DLinkedList<T>::DLinkedList(const DLinkedList<T>& other) :
    m_size(other.m_size)
{
    if (other.m_head == NULL)
    {
        m_head = NULL;
        m_tail = NULL;
    }
    else
    {
        Node* otherIter = other.m_head;
        m_head = new Node(other.m_head->m_data);
        Node* iter = m_head;

        otherIter = otherIter->m_next;

        while (otherIter != NULL)
        {
            iter->m_next = new Node(otherIter->m_data);
            iter->m_next->m_prev = iter;
            iter = iter->m_next;
            otherIter = otherIter->m_next;
        }

        iter->m_next = NULL;
        m_tail = iter;
    }
}

template <typename T>
DLinkedList<T>& DLinkedList<T>::operator=(const DLinkedList<T>& other)
{
    if (this != &other)
    {
        // Delete existing data
        Node* delIter = m_head;

        while (delIter)
        {
            Node* temp = delIter;
            delIter = delIter->m_next;
            delete temp;
        }

        // Copy in new data
        if (other.m_head == NULL)
        {
            m_head = NULL;
        }
        else
        {
            Node* otherIter = other.m_head;
            m_head = new Node(other.m_head->m_data);
            Node* iter = m_head;

            otherIter = otherIter->m_next;

            while (otherIter != NULL)
            {
                iter->m_next = new Node(otherIter->m_data);
                iter->m_next->m_prev = iter;
                iter = iter->m_next;
                otherIter = otherIter->m_next;
            }

            iter->m_next = NULL;
            m_tail = iter;
        }

        m_size = other.m_size;
    }

    return *this;
}

template <typename T>
DLinkedList<T>::DLinkedList(DLinkedList<T>&& other)
{
    m_head = other.m_head;
    m_tail = other.m_tail;
    m_size = other.m_size;

    other.m_head = NULL;
    other.m_tail = NULL;
    other.m_size = 0;
}

template <typename T>
DLinkedList<T>& DLinkedList<T>::operator=(DLinkedList<T>&& other)
{
    if (this != &other)
    {
        m_head = other.m_head;
        m_tail = other.m_tail;
        m_size = other.m_size;

        other.m_head = NULL;
        other.m_tail = NULL;
        other.m_size = 0;
    }

    return *this;
}

template <typename T>
T& DLinkedList<T>::front()
{
    assert(m_head != NULL);

    return m_head->m_data;
}

template <typename T>
const T& DLinkedList<T>::front() const
{
    assert(m_head != NULL);

    return m_head->m_data;
}

template <typename T>
T& DLinkedList<T>::back()
{
    assert(m_head != NULL);

    return m_tail->m_data;
}

template <typename T>
const T& DLinkedList<T>::back() const
{
    assert(m_head != NULL);

    return m_tail->m_data;
}

template <typename T>
void DLinkedList<T>::addFront(const T& value)
{
    if (m_size > 0)
    {
        Node* newNode = new Node(value);
        newNode->m_next = m_head;
        m_head = newNode;
    }
    else
    {
        m_head = new Node(value);
        m_tail = m_head;
    }

    m_size++;
}

template <typename T>
void DLinkedList<T>::addBack(const T& value)
{
    if (m_size > 0)
    {
        Node* newNode = new Node(value);
        newNode->m_prev = m_tail;
        m_tail->m_next = newNode;
        m_tail = newNode;
    }
    else
    {
        m_head = new Node(value);
        m_tail = m_head;
    }

    m_size++;
}

template <typename T>
void DLinkedList<T>::addFront(T&& value)
{
    if (m_size > 0)
    {
        Node* newNode = new Node(std::move(value));
        newNode->m_next = m_head;
        m_head = newNode;
    }
    else
    {
        m_head = new Node(std::move(value));
        m_tail = m_head;
    }

    m_size++;
}

template <typename T>
void DLinkedList<T>::addBack(T&& value)
{
    if (m_size > 0)
    {
        Node* newNode = new Node(std::move(value));
        newNode->m_prev = m_tail;
        newNode->m_next = m_head;
        m_tail->m_next = newNode;
        m_tail = newNode;
    }
    else
    {
        m_head = new Node(std::move(value));
        m_tail = m_head;
    }

    m_size++;
}

template <typename T>
void DLinkedList<T>::popFront()
{
    assert(m_head != NULL);

    Node* delIter = m_head;
    m_head = m_head->m_next;
    delete delIter;
    m_size--;
}

template <typename T>
void DLinkedList<T>::popBack()
{
    assert(m_head != NULL);

    Node* delIter = m_tail;
    m_tail = m_tail->m_prev;
    delete delIter;
    m_size--;
}

template <typename T>
size_t DLinkedList<T>::size() const
{
    return m_size;
}

template <typename T>
typename DLinkedList<T>::ConstIterator DLinkedList<T>::iterator() const
{
    return ConstIterator(m_head);
}

template <typename T>
typename DLinkedList<T>::Iterator DLinkedList<T>::iterator()
{
    return Iterator(m_head);
}

// ConstIterator functions --------------------------------------------------

template <typename T>
DLinkedList<T>::ConstIterator::ConstIterator() :
    m_node(NULL)
{
}

template <typename T>
DLinkedList<T>::ConstIterator::ConstIterator(typename DLinkedList<T>::Node* node) :
    m_node(node)
{
}

template <typename T>
DLinkedList<T>::ConstIterator::ConstIterator(const DLinkedList<T>::ConstIterator& other) :
    m_node(other.m_node)
{
}

template <typename T>
typename DLinkedList<T>::ConstIterator&
    DLinkedList<T>::ConstIterator::operator=(const DLinkedList<T>::ConstIterator& other)
{
    m_node = other.m_node;
    return *this;
}

template <typename T>
bool DLinkedList<T>::ConstIterator::hasPrev() const
{
    return m_node != NULL;
}


template <typename T>
const T& DLinkedList<T>::ConstIterator::prev()
{
    const T& ret = m_node->m_data;
    m_node = m_node->m_prev;
    return ret;
}

template <typename T>
bool DLinkedList<T>::ConstIterator::hasNext() const
{
    return m_node != NULL;
}

template <typename T>
const T& DLinkedList<T>::ConstIterator::next()
{
    const T& ret = m_node->m_data;
    m_node = m_node->m_next;
    return ret;
}

template <typename T>
const T& DLinkedList<T>::ConstIterator::value()
{
    return m_node->m_data;
}

// Iterator functions -------------------------------------------------------

template <typename T>
DLinkedList<T>::Iterator::Iterator() :
    ConstIterator()
{
}

template <typename T>
DLinkedList<T>::Iterator::Iterator(typename DLinkedList<T>::Node* node) :
    ConstIterator(node)
{
}

template <typename T>
DLinkedList<T>::Iterator::Iterator(const DLinkedList<T>::Iterator& other) :
    ConstIterator(other.m_node)
{
}

template <typename T>
typename DLinkedList<T>::Iterator&
    DLinkedList<T>::Iterator::operator=(const DLinkedList<T>::Iterator& other)
{
    this->m_node = other.m_node;
    return *this;
}

template <typename T>
T& DLinkedList<T>::Iterator::prev()
{
    return (T&)ConstIterator::prev();
}

template <typename T>
T& DLinkedList<T>::Iterator::next()
{
    return (T&)ConstIterator::next();
}

template <typename T>
T& DLinkedList<T>::Iterator::value()
{
    return (T&)ConstIterator::value();
}

template <typename T>
void DLinkedList<T>::Iterator::insert(const T& value)
{
    Node* newNode = new Node();

    try
    {
        newNode->m_data = value;
    }
    catch (...)
    {
        delete newNode;
        throw;
    }

    Node* oldPrev = this->m_node->m_prev;
    
    newNode->m_prev = oldPrev;
    oldPrev->m_next = newNode;
    newNode->m_next = this->m_node;
    this->m_node->m_prev = newNode;
}

template <typename T>
void DLinkedList<T>::Iterator::insert(T&& value)
{
    Node* newNode = new Node();
    newNode->m_data = std::forward(value);

    Node* oldPrev = this->m_node->m_prev;
    
    newNode->m_prev = oldPrev;
    oldPrev->m_next = newNode;
    newNode->m_next = this->m_node;
    this->m_node->m_prev = newNode;
}

template <typename T>
void DLinkedList<T>::Iterator::remove()
{
    Node* oldNext = this->m_node->m_next;
    Node* oldPrev = this->m_node->m_prev;

    oldPrev->m_next = oldNext;
    oldNext->m_prev = oldPrev;

    delete this->m_node;
    this->m_node = oldNext;
}

template <typename T>
void DLinkedList<T>::Iterator::set(const T& value)
{
    this->m_node->m_data = value;
}

template <typename T>
void DLinkedList<T>::Iterator::set(T&& value)
{
    this->m_node->m_data = std::forward(value);
}

#endif // DLINKED_LIST_H

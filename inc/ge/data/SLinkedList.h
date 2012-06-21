// SLinkedList.h

#ifndef SLINKED_LIST_H
#define SLINKED_LIST_H

#include <cassert>
#include <memory>

template <typename T>
class SLinkedList
{
private:
    class Node;

public:
    class Iterator;

    SLinkedList();
    SLinkedList(const SLinkedList& other);
    SLinkedList(SLinkedList&& other);
    ~SLinkedList();
    
    SLinkedList& operator=(const SLinkedList& other);
    SLinkedList& operator=(SLinkedList&& other);

    T& back();
    const T& back() const;

    void addBack(const T& value);
    void addBack(T&& value);

    void popBack();

    size_t size() const;

    Iterator iterator();
    const Iterator iterator() const;

    class Iterator
    {
    public:
        friend class SLinkedList;

        Iterator();
        Iterator(const Iterator& other);
        Iterator(Iterator&& other);

        Iterator& operator=(const Iterator& other);
        Iterator& operator=(Iterator&& other);

        bool hasNext();
        T& next();
        const T& next() const;

    private:
        Iterator(Node* node);

        Node* m_node;
    };

private:
    class Node
    {
    public:
        Node() :
            m_data(T()),
            m_next(NULL)
        {
        }

        Node(const T& value) :
            m_data(value),
            m_next(NULL)
        {
        }

        Node(const T&& value) :
            m_data(std::move(value)),
            m_next(NULL)
        {
        }

        T m_data;
        Node* m_next;
    };

    Node* m_head;
    size_t m_size;
};

template <typename T>
SLinkedList<T>::SLinkedList() :
    m_head(NULL),
    m_size(0)
{
}

template <typename T>
SLinkedList<T>::SLinkedList(const SLinkedList<T>& other) :
    m_size(other.m_size)
{
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
            iter = iter->m_next;
            otherIter = otherIter->m_next;
        }

        iter->m_next = NULL;
    }
}

template <typename T>
SLinkedList<T>::SLinkedList(SLinkedList<T>&& other)
{
    m_head = other.m_head;
    m_size = other.m_size;

    other.m_head = NULL;
    other.m_size = 0;
}

template <typename T>
SLinkedList<T>::~SLinkedList()
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
SLinkedList<T>& SLinkedList<T>::operator=(const SLinkedList<T>& other)
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
                iter = iter->m_next;
                otherIter = otherIter->m_next;
            }

            iter->m_next = NULL;
        }

        m_size = other.m_size;
    }

    return *this;
}

template <typename T>
SLinkedList<T>& SLinkedList<T>::operator=(SLinkedList<T>&& other)
{
    if (this != &other)
    {
        m_head = other.m_head;
        m_size = other.m_size;

        other.m_head = NULL;
        other.m_size = 0;
    }

    return *this;
}

template <typename T>
T& SLinkedList<T>::back()
{
    assert(m_head != NULL);

    return m_head->m_data;
}

template <typename T>
const T& SLinkedList<T>::back() const
{
    assert(m_head != NULL);

    return m_head->m_data;
}

template <typename T>
void SLinkedList<T>::addBack(const T& value)
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
    }

    m_size++;
}

template <typename T>
void SLinkedList<T>::addBack(T&& value)
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
    }

    m_size++;
}

template <typename T>
void SLinkedList<T>::popBack()
{
    assert(m_head != NULL);

    Node* delIter = m_head;
    m_head = m_head->m_next;
    delete delIter;
    m_size--;
}

template <typename T>
size_t SLinkedList<T>::size() const
{
    return m_size;
}

template <typename T>
typename SLinkedList<T>::Iterator SLinkedList<T>::iterator()
{
    return Iterator(m_head);
}

template <typename T>
const typename SLinkedList<T>::Iterator SLinkedList<T>::iterator() const
{
    return Iterator(m_head);
}

// Iterator implementation --------------------------------------------------

template <typename T>
SLinkedList<T>::Iterator::Iterator() :
    m_node(NULL)
{
}

template <typename T>
SLinkedList<T>::Iterator::Iterator(typename SLinkedList<T>::Node* node) :
    m_node(node)
{
}

template <typename T>
SLinkedList<T>::Iterator::Iterator(typename const SLinkedList<T>::Iterator& other) :
    m_node(other.m_node)
{
}

template <typename T>
SLinkedList<T>::Iterator::Iterator(typename SLinkedList<T>::Iterator&& other)
{
    m_node = other.m_node;
    other.m_node = NULL;
}

template <typename T>
typename SLinkedList<T>::Iterator&
    SLinkedList<T>::Iterator::operator=(typename const SLinkedList<T>::Iterator& other)
{
    m_node = other.m_node;
    return *this;
}

template <typename T>
typename SLinkedList<T>::Iterator&
    SLinkedList<T>::Iterator::operator=(typename SLinkedList<T>::Iterator&& other)
{
    if (this != &other)
    {
        m_node = other.m_node;
        other.m_node = NULL;
    }

    return *this;
}

template <typename T>
bool SLinkedList<T>::Iterator::hasNext()
{
    return m_node != NULL;
}

template <typename T>
T& SLinkedList<T>::Iterator::next()
{
    T& ret = m_node->m_data;
    m_node = m_node->m_next;
    return ret;
}

template <typename T>
const T& SLinkedList<T>::Iterator::next() const
{
    const T& ret = m_node->m_data;
    m_node = m_node->m_next;
    return ret;
}

#endif // SLINKED_LIST_H

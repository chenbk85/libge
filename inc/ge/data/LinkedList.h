// LinkedList.h

#ifndef SLINKED_LIST_H
#define SLINKED_LIST_H

#include <cassert>

/*
 * Singly linked list.
 */
template <typename T>
class LinkedList
{
private:
    class Node;

public:
    class ConstIterator
    {
        friend class LinkedList;

    public:
        ConstIterator();

        ConstIterator(const ConstIterator& other);
        ConstIterator& operator=(const ConstIterator& other);

        bool hasNext() const;
        const T& next();

        const T& value();

    protected:
        ConstIterator(Node* node);

        Node* m_node;
    };

    class Iterator : public ConstIterator
    {
        friend class LinkedList;

    public:
        Iterator();

        Iterator(const Iterator& other);
        Iterator& operator=(const Iterator& other);

        T& next();
        T& value();
        void set(const T& value);
        void set(T&& value);

    private:
        Iterator(Node* node);
    };

    LinkedList();
    LinkedList(const LinkedList& other);
    LinkedList(LinkedList&& other);
    ~LinkedList();
    
    LinkedList& operator=(const LinkedList& other);
    LinkedList& operator=(LinkedList&& other);

    T& back();
    const T& back() const;

    void addBack(const T& value);
    void addBack(T&& value);

    void popBack();

    size_t size() const;
    bool isEmpty() const;

    ConstIterator iterator() const;
    Iterator iterator();

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
LinkedList<T>::LinkedList() :
    m_head(NULL),
    m_size(0)
{
}

template <typename T>
LinkedList<T>::LinkedList(const LinkedList<T>& other) :
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
LinkedList<T>::LinkedList(LinkedList<T>&& other)
{
    m_head = other.m_head;
    m_size = other.m_size;

    other.m_head = NULL;
    other.m_size = 0;
}

template <typename T>
LinkedList<T>::~LinkedList()
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
LinkedList<T>& LinkedList<T>::operator=(const LinkedList<T>& other)
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
LinkedList<T>& LinkedList<T>::operator=(LinkedList<T>&& other)
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
T& LinkedList<T>::back()
{
    assert(m_head != NULL);

    return m_head->m_data;
}

template <typename T>
const T& LinkedList<T>::back() const
{
    assert(m_head != NULL);

    return m_head->m_data;
}

template <typename T>
void LinkedList<T>::addBack(const T& value)
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
void LinkedList<T>::addBack(T&& value)
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
void LinkedList<T>::popBack()
{
    assert(m_head != NULL);

    Node* delIter = m_head;
    m_head = m_head->m_next;
    delete delIter;
    m_size--;
}

template <typename T>
size_t LinkedList<T>::size() const
{
    return m_size;
}

template <typename T>
bool LinkedList<T>::isEmpty() const
{
    return m_size == 0;
}

template <typename T>
typename LinkedList<T>::ConstIterator LinkedList<T>::iterator() const
{
    return ConstIterator(m_head);
}

template <typename T>
typename LinkedList<T>::Iterator LinkedList<T>::iterator()
{
    return Iterator(m_head);
}

// ConstIterator functions --------------------------------------------------

template <typename T>
LinkedList<T>::ConstIterator::ConstIterator() :
    m_node(NULL)
{
}

template <typename T>
LinkedList<T>::ConstIterator::ConstIterator(typename LinkedList<T>::Node* node) :
    m_node(node)
{
}

template <typename T>
LinkedList<T>::ConstIterator::ConstIterator(const LinkedList<T>::ConstIterator& other) :
    m_node(other.m_node)
{
}

template <typename T>
typename LinkedList<T>::ConstIterator&
    LinkedList<T>::ConstIterator::operator=(const LinkedList<T>::ConstIterator& other)
{
    m_node = other.m_node;
    return *this;
}

template <typename T>
bool LinkedList<T>::ConstIterator::hasNext() const
{
    return m_node != NULL;
}

template <typename T>
const T& LinkedList<T>::ConstIterator::next()
{
    const T& ret = m_node->m_data;
    m_node = m_node->m_next;
    return ret;
}

template <typename T>
const T& LinkedList<T>::ConstIterator::value()
{
    return m_node->m_data;
}

// Iterator functions -------------------------------------------------------

template <typename T>
LinkedList<T>::Iterator::Iterator() :
    ConstIterator()
{
}

template <typename T>
LinkedList<T>::Iterator::Iterator(typename LinkedList<T>::Node* node) :
    ConstIterator(node)
{
}

template <typename T>
LinkedList<T>::Iterator::Iterator(const LinkedList<T>::Iterator& other) :
    ConstIterator(other.m_node)
{
}

template <typename T>
typename LinkedList<T>::Iterator&
    LinkedList<T>::Iterator::operator=(const LinkedList<T>::Iterator& other)
{
    this->m_node = other.m_node;
    return *this;
}

template <typename T>
T& LinkedList<T>::Iterator::next()
{
    return (T&)ConstIterator::next();
}

template <typename T>
T& LinkedList<T>::Iterator::value()
{
    return (T&)ConstIterator::value();
}

template <typename T>
void LinkedList<T>::Iterator::set(const T& value)
{
    this->m_node->m_data = value;
}

template <typename T>
void LinkedList<T>::Iterator::set(T&& value)
{
    this->m_node->m_data = std::forward(value);
}

#endif // SLINKED_LIST_H

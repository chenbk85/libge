// HashMap.h

#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <ge/common.h>
#include <ge/data/Hasher.h>

/*
 * A basic hash table class.
 */
template <typename Key,
          typename Value,
          typename Hasher = hasher<const Key&> >
class HashMap
{
    class TableVal;

public:
    /*
     * Represents an entry in the HashMap.
     */
    class Entry
    {
        friend class HashMap;

    public:
        Entry();

        Entry(Key initKey, Value initValue);

        const Key& getKey() const;

        Value& getValue();
        const Value& getValue() const;

        void set(const Value& value);
        void set(Value&& value);

    private:
        Key _key;
        Value _value;
    };

    /*
     * Const iterator for the HashMap. Forward only.
     */
    class ConstIterator
    {
        friend class HashMap;

    public:
        ConstIterator() :
            _owner(NULL),
            _tableIndex(0),
            _tableVal(0)
        {}

        const Entry& value() const;
        bool isValid() const;

        const Entry& next();
        bool hasNext() const;

    private:
        ConstIterator(const HashMap* owner,
                      size_t tableIndex,
                      TableVal* tableVal) :
            _owner(owner),
            _tableIndex(tableIndex),
            _tableVal(tableVal)
        {
            scrollToValid();
        }

        void scrollToValid();

        const HashMap* _owner;
        size_t _tableIndex;
        TableVal* _tableVal;
    };

    class Iterator : public ConstIterator
    {
        friend class HashMap;

    public:
        Iterator() : ConstIterator()
        {}

        Entry& value() const;
        Entry& next();

    private:
        Iterator(const HashMap* owner,
                 size_t tableIndex,
                 TableVal* tableVal) :
            ConstIterator(owner, tableIndex, tableVal)
        {
        }
    };

    HashMap();
    explicit HashMap(size_t initialSize);
    ~HashMap();

    HashMap(const HashMap& other);
    HashMap& operator=(HashMap& other);

    HashMap(const HashMap&& other);
    HashMap& operator=(HashMap&& other);

    void clear();

    Iterator iterator();
    ConstIterator iterator() const;

    void put(const Key& key, const Value& val);

    Iterator get(const Key& key);
    ConstIterator get(const Key& key) const;

    size_t size() const;

private:
    class TableVal
    {
    public:
        TableVal(Key initKey, Value initValue, uint32 initHash) :
            entry(initKey, initValue),
            hashVal(initHash),
            next(NULL)
        {}

        Entry entry;
        uint32 hashVal;
        TableVal* next;
    };

    void resize(size_t newTableSize);

    TableVal** m_table;
    size_t m_tableSize;
    size_t m_size;
    Hasher m_hasher;
};

template <typename K,
          typename V,
          typename H>
HashMap<K, V, H>::HashMap() :
    m_table(NULL),
    m_tableSize(0),
    m_size(0)
{
}

template <typename K,
          typename V,
          typename H>
HashMap<K, V, H>::HashMap(size_t initialSize) :
    m_tableSize(initialSize),
    m_size(0)
{
    m_table = new TableVal*[initialSize];
}

template <typename K,
          typename V,
          typename H>
HashMap<K, V, H>::~HashMap()
{
    clear();
}

template <typename K,
          typename V,
          typename H>
HashMap<K, V, H>::HashMap(const HashMap& other)
{
    if (other.m_table == NULL)
    {
        m_table = NULL;
        m_tableSize = 0;
        m_size = 0;
    }
    else
    {
        m_table = new TableVal*[other.m_tableSize];
        m_tableSize = other.m_tableSize;
        m_size = other.m_size;

        try
        {
            for (size_t i = 0; i < m_tableSize; i++)
            {
                TableVal* localPtr = NULL;
                TableVal* otherPtr = other.m_table[i];

                while (otherPtr != NULL)
                {
                    TableVal* newTableVal =
                            new TableVal(otherPtr->entry._key,
                                         otherPtr->entry._value,
                                         otherPtr->hashVal);

                    if (localPtr == NULL)
                    {
                        m_table[i] = newTableVal;
                    }
                    else
                    {
                        localPtr->next = newTableVal;
                    }
                    localPtr = newTableVal;
                    otherPtr = otherPtr->next;
                }
            }
        }
        catch (...)
        {
            clear();
            throw;
        }
    }
}

template <typename K,
          typename V,
          typename H>
HashMap<K, V, H>& HashMap<K, V, H>::operator=(HashMap& other)
{
    if (this != &other)
    {
        HashMap copy(other);

        m_table = copy.m_table;
        m_tableSize = copy.m_tableSize;
        m_size = copy.m_size;

        copy.m_table = NULL;
        copy.m_tableSize = 0;
        copy.m_size = 0;
    }

    return *this;
}

template <typename K,
          typename V,
          typename H>
HashMap<K, V, H>::HashMap(const HashMap&& other)
{
    m_table = other.m_table;
    m_tableSize = other.m_tableSize;
    m_size = other.m_size;

    other.m_table = NULL;
    other.m_tableSize = 0;
    other.m_size = 0;
}

template <typename K,
          typename V,
          typename H>
HashMap<K, V, H>& HashMap<K, V, H>::operator=(HashMap&& other)
{
    if (this != &other)
    {
        m_table = other.m_table;
        m_tableSize = other.m_tableSize;
        m_size = other.m_size;

        other.m_table = NULL;
        other.m_tableSize = 0;
        other.m_size = 0;
    }

    return *this;
}

template <typename K,
          typename V,
          typename H>
void HashMap<K, V, H>::clear()
{
    for (size_t i = 0; i < m_tableSize; i++)
    {
        TableVal* tableVal = m_table[i];

        while (tableVal != NULL)
        {
            TableVal* prevTableVal = tableVal;
            tableVal = tableVal->next;
            delete prevTableVal;
        }
    }

    delete[] m_table;

    m_table = NULL;
    m_tableSize = 0;
    m_size = 0;
}

template <typename K,
          typename V,
          typename H>
typename HashMap<K, V, H>::Iterator HashMap<K, V, H>::iterator()
{
    return Iterator(this, 0, m_table[0]);
}

template <typename K,
          typename V,
          typename H>
typename HashMap<K, V, H>::ConstIterator HashMap<K, V, H>::iterator() const
{
    return ConstIterator(this, 0, m_table[0]);
}

template <typename K,
          typename V,
          typename H>
void HashMap<K, V, H>::put(const K& key, const V& value)
{
    if (m_table == NULL)
        resize(16);

    uint32 hashVal = m_hasher(key);
    size_t index = hashVal % m_tableSize;

    TableVal** prevTableVal = &m_table[index];
    TableVal** tableVal = prevTableVal;

    while ((*tableVal) != NULL)
    {
        prevTableVal = tableVal;

        if ((*tableVal)->hashVal == hashVal &&
            (*tableVal)->entry._key == key)
        {
            (*tableVal)->entry._value = value;
            return;
        }

        tableVal = &(*tableVal)->next;
    }

    (*tableVal) = new TableVal(key, value, hashVal);
    m_size++;

    size_t resizeLimit = ((m_tableSize / 2) + (m_tableSize / 4));
    if (m_size > resizeLimit)
    {
        resize(m_tableSize * 2);
    }
}

template <typename K,
          typename V,
          typename H>
typename HashMap<K, V, H>::Iterator HashMap<K, V, H>::get(const K& key)
{
    uint32 hashVal = m_hasher(key);
    size_t index = hashVal % m_tableSize;

    TableVal* tableVal = m_table[index];

    while (tableVal != NULL)
    {
        if (tableVal->hashVal == hashVal &&
            tableVal->entry._key == key)
        {
            return Iterator(this, index, tableVal);
        }
    }

    return Iterator(this, m_tableSize, NULL);
}

template <typename K,
          typename V,
          typename H>
typename HashMap<K, V, H>::ConstIterator HashMap<K, V, H>::get(const K& key) const
{
    uint32 hashVal = m_hasher(key);
    size_t index = hashVal % m_tableSize;

    TableVal* tableVal = m_table[index];

    while (tableVal != NULL)
    {
        if (tableVal->hashVal == hashVal &&
            tableVal->entry._key == key)
        {
            return ConstIterator(this, index, tableVal);
        }
    }

    return ConstIterator(this, m_tableSize, NULL);
}

template <typename K,
          typename V,
          typename H>
size_t HashMap<K, V, H>::size() const
{
    return m_size;
}

// Private functions --------------------------------------------------------

template <typename K,
          typename V,
          typename H>
void HashMap<K, V, H>::resize(size_t newTableSize)
{
    TableVal** newTable = new TableVal*[newTableSize];
    ::memset(newTable, 0, sizeof(TableVal*) * newTableSize);

    for (size_t i = 0; i < m_tableSize; i++)
    {
        TableVal* curVal = m_table[i];

        while (curVal != NULL)
        {
            TableVal* working = curVal;

            size_t newIndex = working->hashVal % newTableSize;

            TableVal* newTableIter = newTable[newIndex];

            if (newTableIter == NULL)
            {
                newTable[newIndex] = working;
            }
            else
            {
                while (newTableIter->next != NULL)
                    newTableIter = newTableIter->next;

                newTableIter->next = working;
            }

            curVal = curVal->next;
            working->next = NULL;
        }
    }

    m_table = newTable;
    m_tableSize = newTableSize;
}

// Entry functions ----------------------------------------------------------

template <typename K,
          typename V,
          typename H>
HashMap<K, V, H>::Entry::Entry() :
    _key(),
    _value()
{}

template <typename K,
          typename V,
          typename H>
HashMap<K, V, H>::Entry::Entry(K initKey, V initValue) :
    _key(initKey),
    _value(initValue)
{}

template <typename K,
          typename V,
          typename H>
const K& HashMap<K, V, H>::Entry::getKey() const
{
    return _key;
}

template <typename K,
          typename V,
          typename H>
V& HashMap<K, V, H>::Entry::getValue()
{
    return _value;
}

template <typename K,
          typename V,
          typename H>
const V& HashMap<K, V, H>::Entry::getValue() const
{
    return _value;
}

template <typename K,
          typename V,
          typename H>
void HashMap<K, V, H>::Entry::set(const V& value)
{
    _value = value;
}

template <typename K,
          typename V,
          typename H>
void HashMap<K, V, H>::Entry::set(V&& value)
{
    _value = std::forward(value);
}

// ConstIterator functions --------------------------------------------------

template <typename K,
          typename V,
          typename H>
typename const HashMap<K, V, H>::Entry&
    HashMap<K, V, H>::ConstIterator::value() const
{
    return _tableVal->entry;
}

template <typename K,
          typename V,
          typename H>
bool HashMap<K, V, H>::ConstIterator::isValid() const
{
    return (_tableVal != NULL);
}

template <typename K,
          typename V,
          typename H>
typename const HashMap<K, V, H>::Entry&
    HashMap<K, V, H>::ConstIterator::next()
{
    TableVal* ret = _tableVal;
    _tableVal = _tableVal->next;
    scrollToValid();
    return ret->entry;
}

template <typename K,
          typename V,
          typename H>
bool HashMap<K, V, H>::ConstIterator::hasNext() const
{
    return (_tableIndex < _owner->m_tableSize &&
            _tableVal != NULL);
}

template <typename K,
          typename V,
          typename H>
void HashMap<K, V, H>::ConstIterator::scrollToValid()
{
    if (_tableVal != NULL)
    {
        return;
    }

    // Start searching the table
    while (_tableVal == NULL &&
           _tableIndex < _owner->m_tableSize)
    {
        _tableIndex++;
        _tableVal = _owner->m_table[_tableIndex];
    }

    // We either hit a valid value or end of table
}

// Iterator functions -------------------------------------------------------

template <typename K,
          typename V,
          typename H>
typename HashMap<K, V, H>::Entry&
    HashMap<K, V, H>::Iterator::value() const
{
    return (Entry&)ConstIterator::value();
}

template <typename K,
          typename V,
          typename H>
typename HashMap<K, V, H>::Entry&
    HashMap<K, V, H>::Iterator::next()
{
    return (Entry&)ConstIterator::next();
}

#endif // HASH_MAP_H

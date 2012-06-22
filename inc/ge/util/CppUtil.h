// CppUtil

#ifndef CPP_UTIL_H
#define CPP_UTIL_H

#include <ge/common.h>

#include <cstddef>
#include <cstdlib>
#include <new>
#include <utility>

/*
 * Namespace for simple C++ utility functions.
 */
namespace CppUtil
{
    /*
     * Allocates an uninitialized block of count objects that are aligned
     * to the alignment of T reported by the compilter. Throws std::bad_alloc
     * on failure to allocate.
     *
     * Make sure you manually destroy any items in the returned buffer and
     * free with uninitializedFree if you allocate with this function as
     * delete or delete[] may not succeed.
     */
    template <typename T>
    T* uninitializedAlloc(size_t count)
    {
        T* mem = (T*)::malloc(sizeof(T) * count);

        if (!mem)
            throw std::bad_alloc();

        return mem;
    }

    /*
     * Frees memory allocated with uninitizedAlloc. Does nothing if passed a
     * null pointer.
     */
    template <typename T>
    void uninitializedFree(T* ptr)
    {
        if (ptr)
           ::free(ptr);
    }

    /*
     * Default constructs the object at the passed location. The location
     * should either be uninitialized or have been destroyed previously.
     */
    template <typename T>
    void defaultConstruct(T* target)
    {
        new (target) T();
    }

    /*
     * Copy constructs the object at the passed location, using the passed
     * value. The location should either be uninitialized or have been
     * destroyed previously.
     */
    template <typename T>
    void copyConstruct(T* target, const T& sourceObj)
    {
        new (target) T(sourceObj);
    }

    /*
     * Move constructs the object at the passed location, using the passed
     * value. The location should either be uninitialized or have been
     * destroyed previously.
     */
    template <typename T>
    void moveConstruct(T* target, T&& sourceObj)
    {
        new (target) T(std::forward<T&&>(sourceObj));
    }

    /*
     * Destroys the object at the passed location.
     */
    template <typename T>
    void destroy(T* ptr)
    {
        ((T*)ptr)->~T();
    }

    /*
     * Destroys an array of objects.
     */
    template <typename T>
    void destroy(T* start, T* end)
    {
        while (start != end)
        {
            start->~T();
            start++;
        }
    }

    /*
     * Hard rounds down a pointer to the given alignment. The type may not be
     * valid at the alignment.
     */
    template <typename T>
    T* roundDownPtr(T* ptr, size_t align)
    {
        return (T*)((size_t)ptr - ((size_t)ptr % align));
    }
}

#endif // CPP_UTIL_H

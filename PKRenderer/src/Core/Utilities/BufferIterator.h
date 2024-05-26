#pragma once
#include <iterator>

namespace PK
{
    template<typename T>
    struct ConstBufferIterator
    {
        struct Node
        {
            T const* value;
            size_t index;
            Node(T const* value, size_t index = 0ull) : value(value), index(index) {}
            T const* operator->() const { return value; }
            T const& operator*() const { return *value; }
        };

        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Node;
        using pointer = const Node*;
        using reference = const Node&;

        ConstBufferIterator(T const* value, size_t index) : data(value), index(index), node(value, index) {}

        reference operator*() const { return node; }
        pointer operator->() const { return &node; }
        
        T const* data;
        size_t index;
        Node node;
    };

    template<typename T>
    static constexpr bool operator != (const ConstBufferIterator<T>& a, const ConstBufferIterator<T>& b)
    { 
        return a.data != b.data;
    }

    template<typename T>
    static constexpr ConstBufferIterator<T>& operator ++ (ConstBufferIterator<T>& a)
    { 
        a.data++;
        a.index++;
        a.node = typename ConstBufferIterator<T>::Node(a.data, a.index);
        return a;
    }

    template<typename T>
    struct BufferIterator
    {
        struct Node
        {
            T* value;
            size_t index;
            Node(T* value, size_t index = 0ull) : value(value), index(index) {}
            T* operator->() { return value; }
            T& operator*() { return *value; }
        };

        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Node;
        using pointer = Node*;
        using reference = Node&;

        BufferIterator(T* value, size_t index) : data(value), index(index), node(value, index) {}

        reference operator*() { return node; }
        pointer operator->() { return &node; }
        
        T* data;
        size_t index;
        Node node;
    };

    template<typename T>
    static constexpr bool operator != (const BufferIterator<T>& a, const BufferIterator<T>& b)
    { 
        return a.data != b.data;
    }

    template<typename T>
    static constexpr BufferIterator<T>& operator ++ (BufferIterator<T>& a)
    { 
        a.data++;
        a.index++;
        a.node = typename BufferIterator<T>::Node(a.data, a.index);
        return a;
    }
}
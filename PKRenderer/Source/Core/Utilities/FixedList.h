#pragma once
#include "NoCopy.h"
#include "BufferIterator.h"
#include "BufferView.h"
#include <exception>

namespace PK
{
    template<typename T, size_t capacity>
    class FixedList : NoCopy
    {
        using alloc_rebind = typename std::allocator_traits<std::allocator<T>>::template rebind_alloc<T>;
        using alloc_traits = std::allocator_traits<alloc_rebind>; 

    public:
        FixedList()
        {
            m_data = alloc_traits::allocate(m_alloc, capacity);
            m_count = 0ull;
        }

        FixedList(const T* elements, size_t count)
        {
            if (count >= capacity)
            {
                throw std::exception("FixedList capacity exceeded!");
            }

            m_data = alloc_traits::allocate(m_alloc, capacity);
            m_count = count;
            memcpy(m_data, elements, sizeof(T) * count);
        }

        FixedList(std::initializer_list<T> elements) : FixedList(elements.begin(), (size_t)(elements.end() - elements.begin()))
        {
        }

        ~FixedList()
        {
            for (auto i = 0u; i < m_count; ++i)
            {
                alloc_traits::destroy(m_alloc, m_data + i);
            }

            alloc_traits::deallocate(m_alloc, m_data, capacity);
        }

        T* Add()
        {
            if (m_count >= capacity)
            {
                throw std::exception("FixedList capacity exceeded!");
            }

            auto ptr = m_data + m_count++;
            alloc_traits::construct(m_alloc, ptr);
            return ptr;
        }

        template<typename ... Args>
        T* Add(Args&& ... args)
            {
                if (m_count >= capacity)
                {
                    throw std::exception("FixedList capacity exceeded!");
                }

                auto ptr = m_data + m_count++;
                alloc_traits::construct(m_alloc, ptr, std::forward<Args>(args)...);
                return ptr;
            }

        void Pop()
        {
            if (m_count > 0)
            {
                m_count--;
                alloc_traits::destroy(m_alloc, m_data + m_count);
            }
        }

        T& operator [](size_t i) 
        {
            if (i >= m_count)
            {
                for (size_t j = 0u, k = i - m_count; j <= k; ++j)
                {
                    Add();
                }
            }

            return m_data[i];
        }

        const T& operator [](size_t i) const { return m_data[i]; }

        constexpr size_t GetCount() const { return m_count; }

        void SetCount(size_t count) { m_count = count; }

        void Clear() { SetCount(0u); }

        void ClearFull() 
        {
            for (auto i = 0u; i < m_count; ++i)
            {
                alloc_traits::destroy(m_alloc, m_data + i);
            }

            SetCount(0u);
        }

        constexpr const T* GetData() const { return m_data; }
        T* GetData() { return m_data; }

        ConstBufferIterator<T> begin() const { return ConstBufferIterator<T>(m_data, 0ull); }
        ConstBufferIterator<T> end() const { return ConstBufferIterator<T>(m_data + m_count, m_count); }

        BufferIterator<T> begin() { return BufferIterator<T>(m_data, 0ull); }
        BufferIterator<T> end() { return BufferIterator<T>(m_data + m_count, m_count); }

        BufferView<T> GetView() { return { m_data, m_count }; }

    private:
        T* m_data;
        size_t m_count;
        alloc_rebind m_alloc;
    };

    template<typename T, size_t capacity>
    struct InlineList
    {
        InlineList() { m_count = 0ull; }

        InlineList(const T* elements, size_t count)
        {
            if (count >= capacity)
            {
                throw std::exception("FixedList capacity exceeded!");
            }

            m_count = count;
            std::copy(elements, elements + count, reinterpret_cast<T*>(m_data));
        }

        InlineList(std::initializer_list<T> elements) : InlineList(elements.begin(), (size_t)(elements.end() - elements.begin()))
        {
        }

        ~InlineList()
        {
            for (auto i = 0u; i < m_count; ++i)
            {
                (reinterpret_cast<T*>(m_data) + i)->~T();
            }
        }

        T* Add()
        {
            if (m_count >= capacity)
            {
                throw std::exception("FixedList capacity exceeded!");
            }

            auto ptr = reinterpret_cast<T*>(m_data) + m_count++;
            new(ptr) T();
            return ptr;
        }

        template<typename ... Args>
        T* Add(Args&& ... args)
        {
            if (m_count >= capacity)
            {
                throw std::exception("FixedList capacity exceeded!");
            }

            auto ptr = reinterpret_cast<T*>(m_data) + m_count++;
            new(ptr) T(std::forward<Args>(args)...);
            return ptr;
        }

        void Pop()
        {
            if (m_count > 0)
            {
                m_count--;
                (reinterpret_cast<T*>(m_data) + m_count)->~T();
            }
        }

        T& operator [](size_t i) 
        {
            if (i >= m_count)
            {
                for (size_t j = 0u, k = i - m_count; j <= k; ++j)
                {
                    Add();
                }
            }

            return reinterpret_cast<T*>(m_data)[i];
        }

        const T& operator [](size_t i) const { return reinterpret_cast<const T*>(m_data)[i]; }

        constexpr size_t GetCount() const { return m_count; }

        void SetCount(size_t count) { m_count = count; }

        void Clear() { SetCount(0u); }

        void ClearFull() 
        {
            for (auto i = 0u; i < m_count; ++i)
            {
                (reinterpret_cast<T*>(m_data) + i)->~T();
            }

            SetCount(0u);
        }

        constexpr const T* GetData() const { return reinterpret_cast<const T*>(m_data); }
        T* GetData() { return reinterpret_cast<T*>(m_data); }

        ConstBufferIterator<T> begin() const { return ConstBufferIterator<T>(reinterpret_cast<const T*>(m_data), 0ull); }
        ConstBufferIterator<T> end() const { return ConstBufferIterator<T>(reinterpret_cast<const T*>(m_data) + m_count, m_count); }

        BufferIterator<T> begin() { return BufferIterator<T>(reinterpret_cast<T*>(m_data), 0ull); }
        BufferIterator<T> end() { return BufferIterator<T>(reinterpret_cast<T*>(m_data) + m_count, m_count); }

        BufferView<T> GetView() { return { reinterpret_cast<T*>(m_data), m_count }; }

    private:
        alignas(T) std::byte m_data[sizeof(T) * capacity];
        size_t m_count;
    };
}
#pragma once
#include "NoCopy.h"
#include "BufferIterator.h"
#include <exception>

namespace PK::Utilities
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
                    throw std::exception("FList capacity exceeded!");
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
                    throw std::exception("FList capacity exceeded!");
                }

                auto ptr = m_data + m_count++;
                alloc_traits::construct(m_alloc, ptr, std::forward<Args>(args)...);
                return ptr;
            }

            void Pop()
            {
                if (m_count == 0)
                {
                    return;
                }

                m_count--;
                alloc_traits::destroy(m_alloc, m_data + m_count);
            }

            T* operator [](size_t i) 
            {
                if (i >= m_count)
                {
                    for (size_t j = 0u, k = i - m_count; j <= k; ++j)
                    {
                        Add();
                    }
                }

                return m_data + i; 
            }

            constexpr size_t GetCount() const { return m_count; }

            ConstBufferIterator<T> begin() const { return ConstBufferIterator<T>(m_data, 0ull); }
            ConstBufferIterator<T> end() const { return ConstBufferIterator<T>(m_data + m_count, m_count); }

            BufferIterator<T> begin() { return BufferIterator<T>(m_data, 0ull); }
            BufferIterator<T> end() { return BufferIterator<T>(m_data + m_count, m_count); }

        private:
            T* m_data;
            size_t m_count;
            alloc_rebind m_alloc;
    };
}
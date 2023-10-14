#pragma once
#include "NoCopy.h"
#include "Bitmask.h"
#include <vector>
#include <exception>

namespace PK::Utilities
{
    template<typename T, size_t capacity>
    class FixedPool : NoCopy
    {
        using alloc_rebind = typename std::allocator_traits<std::allocator<T>>::template rebind_alloc<T>;
        using alloc_traits = std::allocator_traits<alloc_rebind>; 

        public:
            FixedPool() : m_mask()
            {
                m_data = alloc_traits::allocate(m_alloc, capacity);
            }

            ~FixedPool()
            {
                Clear();
                alloc_traits::deallocate(m_alloc, m_data, capacity);
            }

            T* operator [](uint32_t index) { return m_data + index; }

            const T* operator [](uint32_t index) const { return m_data + index; }

            uint32_t GetIndex(const T* ptr) const 
            {
                if (ptr < m_data || ptr >= (m_data + capacity))
                {
                    throw std::exception("Trying to delete an element that doesn't belong to the pool");
                }

                return (uint32_t)(ptr - m_data);
            }

            template<typename ... Args>
            T* NewAt(int64_t index, Args&& ... args)
            {
                if (index == -1 || index >= capacity)
                {
                    throw std::exception("Pool capacity exceeded!");
                }

                auto ptr = m_data + index;
                m_mask.ToggleAt(index);
                alloc_traits::construct(m_alloc, ptr, std::forward<Args>(args)...);
                return ptr;
            }

            template<typename ... Args>
            T* New(Args&& ... args)
            {
                return NewAt(m_mask.FirstFalse(), std::forward<Args>(args)...);
            }

            void Delete(const T* ptr)
            {
                auto index = GetIndex(ptr);
                if (m_mask.GetAt(index))
                {
                    m_mask.ToggleAt(index);
                    alloc_traits::destroy(m_alloc, ptr);
                }
            }

            void Delete(uint32_t index)
            {
                Delete(m_data + index);
            }

            void Delete(const Bitmask<capacity>& mask)
            {
                // if a whole 64 bit segment is empty we can assume that the rest are as well.
                // Could lead to issues in a very unlucky scenario.
                for (auto i = 0; i < mask.Size && mask.m_mask[i] != 0ull; ++i)
                {
                    for (uint32_t j = i * mask.Stride, k = j + mask.Stride; j < k; ++j)
                    {
                        if (mask.GetAt(j))
                        {
                            Delete(j);
                        }
                    }
                }
            }

            void Clear()
            {
                for (auto i = 0u; i < capacity; ++i)
                {
                    if (m_mask.GetAt(i))
                    {
                        alloc_traits::destroy(m_alloc, m_data + i);
                        m_mask.ToggleAt(i);
                    }
                }
            }

            const Bitmask<capacity>& GetActiveMask() { return m_mask; }

        private:
            T* m_data;
            Bitmask<capacity> m_mask;
            alloc_rebind m_alloc;
    };
}
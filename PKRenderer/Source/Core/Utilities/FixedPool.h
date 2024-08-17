#pragma once
#include "NoCopy.h"
#include "Bitmask.h"
#include <exception>

namespace PK
{
    template<typename T, size_t capacity>
    struct FixedPool : NoCopy
    {
        FixedPool() : m_mask() {}
        ~FixedPool() { Clear(); }

        constexpr const T* GetData() const { return reinterpret_cast<const T*>(m_data); }
        T* GetData() { return reinterpret_cast<T*>(m_data); }

        const Bitmask<capacity>& GetActiveMask() { return m_mask; }
        size_t GetCount() { return m_mask.CountBits(); }

        T* operator [](uint32_t index) { return GetData() + index; }
        const T* operator [](uint32_t index) const { return GetData() + index; }


        uint32_t GetIndex(const T* ptr) const 
        {
            if (ptr < GetData() || ptr >= (GetData() + capacity))
            {
                throw std::exception("Trying to delete an element that doesn't belong to the pool");
            }

            return (uint32_t)(ptr - GetData());
        }

        template<typename ... Args>
        T* NewAt(int64_t index, Args&& ... args)
        {
            if (index == -1 || (size_t)index >= capacity)
            {
                throw std::exception("Pool capacity exceeded!");
            }

            auto ptr = GetData() + index;
            m_mask.ToggleAt(index);
            new(ptr) T(std::forward<Args>(args)...);
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
                ptr->~T();
            }
        }

        void Delete(uint32_t index)
        {
            Delete(GetData() + index);
        }

        void Delete(const Bitmask<capacity>& mask)
        {
            // if a whole 64 bit segment is empty we can assume that the rest are as well.
            // Could lead to issues in a very unlucky scenario.
            for (auto i = 0u; i < mask.Size && mask.m_mask[i] != 0ull; ++i)
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
                    (GetData() + i)->~T();
                    m_mask.ToggleAt(i);
                }
            }
        }

        private:
            alignas(T) std::byte m_data[sizeof(T) * capacity];
            Bitmask<capacity> m_mask;
    };
}
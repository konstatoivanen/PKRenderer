#pragma once
#include <initializer_list>
#include "Memory.h"

namespace PK
{
    template<typename T, size_t capacity>
    struct FixedArray
    {
        FixedArray()
        {
        }

        FixedArray(const T* elements, size_t count)
        {
            if (count < capacity)
            {
                Memory::CopyArray(m_array, elements, count);
            }
        }

        FixedArray(std::initializer_list<T> elements) : FixedArray(elements.begin(), (size_t)(elements.end() - elements.begin()))
        {
        }

        T& operator [](size_t i) { return m_array[i]; }
        const T& operator [](size_t i) const { return m_array[i]; }

        void Clear() { Memory::Memset<T>(m_array, 0, capacity); }

    private:
        T m_array[capacity]{};
    };
}

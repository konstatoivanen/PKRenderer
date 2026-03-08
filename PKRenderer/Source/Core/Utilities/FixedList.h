#pragma once
#include "NoCopy.h"
#include "BufferView.h"
#include "Memory.h"

namespace PK
{
    template<typename T, size_t capacity>
    struct FixedList
    {
        FixedList() { m_count = 0ull; }

        FixedList(const T* elements, size_t count)
        {
            PK_CONTAINER_RANGE_CHECK(count, 0u, capacity);
            m_count = count;
            std::copy(elements, elements + count, reinterpret_cast<T*>(m_data));
        }

        FixedList(std::initializer_list<T> elements) : FixedList(elements.begin(), (size_t)(elements.end() - elements.begin()))
        {
        }

        ~FixedList() { Memory::ClearArray(GetData(), m_count); }

        T* GetData() { return reinterpret_cast<T*>(m_data); }
        constexpr T const* GetData() const { return reinterpret_cast<T const*>(m_data); }

        constexpr size_t GetCount() const { return m_count; }
        constexpr size_t GetSize() const { return m_count * sizeof(T); }

        BufferView<T> GetView() { return { GetData(), m_count }; }
        constexpr ConstBufferView<T> GetView() const { return { GetData(), m_count }; }

        T* begin() { return GetData(); }
        T* end() { return GetData() + m_count; }
        constexpr T const* begin() const { return GetData(); }
        constexpr T const* end() const { return GetData() + m_count; }
        
        T& operator [](size_t i) 
        {
            for (; m_count <= i;) Add();
            return GetData()[i];
        }
        
        T const& operator [](size_t i) const { return GetData()[i]; }

        T* Add()
        {
            PK_CONTAINER_RANGE_CHECK(m_count, 0u, capacity);
            auto ptr = GetData() + m_count++;
            new(ptr) T();
            return ptr;
        }

        template<typename ... Args>
        T* Add(Args&& ... args)
        {
            PK_CONTAINER_RANGE_CHECK(m_count, 0u, capacity);
            auto ptr = GetData() + m_count++;
            new(ptr) T(std::forward<Args>(args)...);
            return ptr;
        }

        void SetCount(size_t count) { m_count = count; }

        void ClearFast() { SetCount(0u); }

        void Clear() 
        {
            Memory::ClearArray(GetData(), m_count);
            SetCount(0u);
        }

    private:
        alignas(T) uint8_t m_data[sizeof(T) * capacity];
        size_t m_count;
    };
}

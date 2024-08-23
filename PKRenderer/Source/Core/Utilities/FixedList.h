#pragma once
#include "NoCopy.h"
#include "BufferIterator.h"
#include "BufferView.h"
#include "ContainerHelpers.h"
#include <exception>

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

        ~FixedList()
        {
            for (auto i = 0u; i < m_count; ++i)
            {
                (reinterpret_cast<T*>(m_data) + i)->~T();
            }
        }

        constexpr size_t GetCount() const { return m_count; }

        constexpr const T* GetData() const { return reinterpret_cast<const T*>(m_data); }
        T* GetData() { return reinterpret_cast<T*>(m_data); }

        BufferView<T> GetView() { return { GetData(), m_count }; }

        ConstBufferIterator<T> begin() const { return ConstBufferIterator<T>(GetData(), 0ull); }
        ConstBufferIterator<T> end() const { return ConstBufferIterator<T>(GetData() + m_count, m_count); }
        
        BufferIterator<T> begin() { return BufferIterator<T>(GetData(), 0ull); }
        BufferIterator<T> end() { return BufferIterator<T>(GetData() + m_count, m_count); }
        
        const T& operator [](size_t i) const { return GetData()[i]; }
        

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

        void Pop()
        {
            if (m_count > 0)
            {
                m_count--;
                (GetData() + m_count)->~T();
            }
        }

        void SetCount(size_t count) { m_count = count; }

        void Clear() { SetCount(0u); }

        void ClearFull() 
        {
            for (auto i = 0u; i < m_count; ++i)
            {
                (GetData() + i)->~T();
            }

            SetCount(0u);
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

            return GetData()[i];
        }

    private:
        alignas(T) std::byte m_data[sizeof(T) * capacity];
        size_t m_count;
    };
}
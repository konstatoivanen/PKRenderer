#pragma once
#include "NoCopy.h"
#include "Memory.h"
#include "BufferView.h"
#include "InitializerList.h"

namespace PK
{
    template<typename T, size_t fixed_count = 0ull>
    struct List : NoCopy
    {
        List() {}
        List(size_t capacity) { Reserve(capacity); }
        List(List&& other) noexcept { Move(PK::Forward<FastBuffer>(other)); }
        List(const List& other) noexcept { Copy(other); }
        List(initializer_list<T>&& other) noexcept { Move(PK::Forward<initializer_list<T>>(other)); }
        List(const initializer_list<T>& other) noexcept { Copy(other); }

        ~List()
        {
            Clear();

            if (!IsSmallBuffer(m_capacity))
            {
                Memory::Free(m_data.buffer);
            }

            m_capacity = 0u;
            m_data.buffer = nullptr;
        }

        T* GetData() { return IsSmallBuffer(m_capacity) ? reinterpret_cast<T*>(&m_data) : m_data.buffer; }
        constexpr T const* GetData() const { return IsSmallBuffer(m_capacity) ? reinterpret_cast<T const*>(&m_data) : m_data.buffer; }

        constexpr size_t GetCount() const { return m_count; }
        constexpr size_t GetSize() const { return m_count * sizeof(T); }

        BufferView<T> GetView() { return { GetData(), m_count }; }
        constexpr ConstBufferView<T> GetView() const { return { GetData(), m_count }; }

        T* begin() { return GetData(); }
        T* end() { return GetData() + m_count; }
        constexpr T const* begin() const { return GetData(); }
        constexpr T const* end() const { return GetData() + m_count; }

        T& operator [](size_t i) { return GetData()[i]; }
        T const& operator [](size_t i) const { return GetData()[i]; }

        operator T* () { return GetData(); }
        operator T const* () const { return GetData(); }

        List& operator=(List&& other) noexcept { Move(PK::Forward<FastBuffer>(other)); return *this; }

        void Copy(const List& other)
        {
            Clear();
            Reserve(other.m_count);
            Memory::CopyArray(GetData(), other.GetData(), other.m_count);
        }

        void Copy(const initializer_list<T>& initializer)
        {
            Resize(initializer.size());
            Memory::CopyArray(GetData(), initializer.begin(), initializer.size());
        }

        void Move(List&& other)
        {
            if (this != &other)
            {
                if (!IsSmallBuffer(m_capacity))
                {
                    Memory::Free(m_data.buffer);
                }

                m_data = PK::Exchange(other.m_data, { nullptr });
                m_count = PK::Exchange(other.m_count, 0ull);
                m_capacity = PK::Exchange(other.m_capacity, 0ull);
            }
        }

        void Move(initializer_list<T>&& initializer)
        {
            Reserve(initializer.size());
            Memory::MoveArray(GetData(), initializer.begin(), initializer.size());
            m_count = initializer.size();
        }

        bool Reserve(size_t capacity)
        {
            if (capacity <= m_capacity)
            {
                return false;
            }

            auto buffer = Memory::Allocate<T>(capacity);

            if (m_count > 0u)
            {
                Memory::MoveArray(buffer, GetData(), m_count);
            }

            if (!IsSmallBuffer(m_capacity))
            {
                Memory::Free(m_data.buffer);
            }

            m_data.buffer = buffer;
            m_capacity = capacity;
            return true;
        }

        void Resize(size_t count)
        {
            Reserve(count);

            while (m_count < count)
            {
                Memory::Construct(GetData() + m_count++);
            }
        }

        void Clear() 
        {
            Memory::ClearArray(GetData(), m_count);
            m_count = 0u;
        }

        template<typename ... Args>
        T* Add(Args&& ... args)
        {
            Reserve(m_count + 1u);
            return Memory::Construct(GetData() + m_count++, PK::Forward<Args>(args)...);
        }

        bool Remove(T* ptr)
        {
            return RemoveAt((uint32_t)(ptr - GetData()));
        }

        bool RemoveAt(uint32_t i)
        {
            if (i >= m_count)
            {
                return false;
            }

            if (i == --m_count)
            {
                Memory::Destruct(GetData() + i);
                return true;
            }

            for (; i < m_count; ++i)
            {
                GetData()[i] = PK::MoveTemp(GetData()[i + 1]);
            }

            return true;
        }

        bool UnorderedRemoveAt(uint32_t i)
        {
            if (i >= m_count)
            {
                return false;
            }

            if (i == --m_count)
            {
                Memory::Destruct(GetData() + i);
                return true;
            }

            if (m_count > 0u)
            {
                GetData()[i] = PK::MoveTemp(GetData()[m_count]);
                return true;
            }

            return false;
        }

    private:
        constexpr static bool IsSmallBuffer(size_t count) { return count <= (sizeof(U) / sizeof(T)); }
        struct U { union { T* buffer; alignas(T) uint8_t inl[fixed_count > 0ull ? sizeof(T) * fixed_count : sizeof(T*)]; }; };
        U m_data{};
        uint32_t m_count = 0u;
        uint32_t m_capacity = sizeof(U) / sizeof(T);
    };

    template<typename T, size_t capacity>
    struct FixedList
    {
        FixedList() { m_count = 0ull; }

        FixedList(const T* elements, size_t count)
        {
            Memory::Assert(count < capacity, "Fixed list capacity exceeded!");
            Memory::CopyArray(reinterpret_cast<T*>(m_data), elements, count);
            m_count = count;
        }

        FixedList(initializer_list<T> elements) : FixedList(elements.begin(), (size_t)(elements.end() - elements.begin()))
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
            Memory::Assert(m_count < capacity, "Fast list capacity exceeded!");
            return Memory::Construct(GetData() + m_count++);
        }

        template<typename ... Args>
        T* Add(Args&& ... args)
        {
            Memory::Assert(m_count < capacity, "Fast list capacity exceeded!");
            return Memory::Construct(GetData() + m_count++, PK::Forward<Args>(args)...);
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

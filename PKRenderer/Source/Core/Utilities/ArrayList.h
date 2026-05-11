#pragma once
#include "NoCopy.h"
#include "BufferView.h"
#include "InitializerList.h"
#include "Allocation.h"

namespace PK
{
    /// <summary>
    /// A non owning container. dont use for types that need implicit destructors.
    /// </summary>
    template<typename T, typename TAllocation>
    struct Array : NoCopy
    {
        using TData = typename TAllocation::template Data<T>;

        constexpr Array() : m_data() {}
        Array(size_t capacity) noexcept : Array() { Reserve(capacity, false); }
        Array(Array&& other) noexcept : Array() { Move(PK::Forward<Array>(other)); }
        Array(initializer_list<T>&& other) noexcept : Array() { Move(other.begin(), other.size())); }
        Array(const Array& other) noexcept : Array() { Copy(other.GetData(), other.GetCount()); }
        Array(const initializer_list<T>& other) noexcept : Array() { Copy(other.begin(), other.size()); }
        Array(const T* elements, size_t count) noexcept : Array() { Copy(elements, count); }
        ~Array() { TData::Free(m_data); }

        constexpr T* GetData() { return TData::GetPtr(m_data); }
        constexpr T const* GetData() const { return TData::GetPtr(m_data); }
        constexpr size_t GetCount() const { return TData::GetCount(m_data); }
        constexpr size_t GetSize() const { return TData::GetSize(m_data); }

        constexpr BufferView<T> GetView() { return { GetData(), GetCount() }; }
        constexpr ConstBufferView<T> GetView() const { return { GetData(), GetCount() }; }

        T* begin() { return GetData(); }
        T* end() { return GetData() + GetCount(); }
        constexpr T const* begin() const { return GetData(); }
        constexpr T const* end() const { return GetData() + GetCount(); }
        
        T& operator [](size_t i) { return GetData()[i]; }
        T const& operator [](size_t i) const { return GetData()[i]; }
        operator T* () { return GetData(); }
        operator T const* () const { return GetData(); }
        Array& operator=(Array&& other) noexcept { Move(PK::Forward<Array>(other)); return *this; }
        Array& operator=(const Array& other) noexcept { Copy(other); return *this; }

        inline void Copy(const T* elements, size_t count)
        {
            Reserve(count, false);
            Memory::CopyArray(GetData(), elements, count);
        }

        inline void Copy(const Array& other)
        {
            Copy(other.GetData(), other.GetCount());
        }

        inline void Move(const T* elements, size_t count)
        {
            Reserve(count);
            Memory::MoveArray(GetData(), elements, count);
        }

        inline void Move(Array&& other)
        {
            if (this != &other)
            {
                TData::Free(m_data);
                m_data = PK::Exchange(other.m_data, {});
            }
        }

        bool Reserve(size_t newCount, bool preserve)
        {
            if (newCount > TData::GetCount(m_data))
            {
                auto newData = TData::Allocate(newCount);

                if (preserve && GetCount() > 0u)
                {
                    Memory::Memcpy<T>(TData::GetPtr(newData), TData::GetPtr(m_data), GetCount());
                }

                TData::Free(m_data);
                m_data = newData;
                return true;
            }

            return false;
        }

        void Clear() 
        {
            Memory::Memset<T>(GetData(), 0, GetCount()); 
        }

    private:
        TData m_data;
    };

    /// <summary>
    /// An owning container. Prefer over array if you want lifetime management.
    /// </summary>
    template<typename T, typename TAllocation>
    struct List : NoCopy
    {
        using TData = typename TAllocation::template Data<T>;

        constexpr List() : m_data(), m_count(0ull) {}
        List(size_t capacity) noexcept : List() { Reserve(capacity); }
        List(List&& other) noexcept : List() { Move(PK::Forward<List>(other)); }
        List(initializer_list<T>&& other) noexcept : List() { Move(other.begin(), other.size())); }
        List(const List& other) noexcept : List() { Copy(other.GetData(), other.GetCount()); }
        List(const initializer_list<T>& other) noexcept : List() { Copy(other.begin(), other.size()); }
        List(const T* elements, size_t count) noexcept : List() { Copy(elements, count); }
        ~List() { Clear(); TData::Free(m_data); }

        constexpr T* GetData() { return TData::GetPtr(m_data); }
        constexpr T const* GetData() const { return TData::GetPtr(m_data); }
        constexpr size_t GetCount() const { return m_count; }
        constexpr size_t GetSize() const { return m_count * sizeof(T); }

        constexpr BufferView<T> GetView() { return { GetData(), m_count }; }
        constexpr ConstBufferView<T> GetView() const { return { GetData(), m_count }; }

        T* begin() { return GetData(); }
        T* end() { return GetData() + m_count; }
        constexpr T const* begin() const { return GetData(); }
        constexpr T const* end() const { return GetData() + m_count; }

        T& operator [](size_t i) { return GetData()[i]; }
        T const& operator [](size_t i) const { return GetData()[i]; }
        operator T* () { return GetData(); }
        operator T const* () const { return GetData(); }
        List& operator=(List&& other) noexcept { Move(PK::Forward<List>(other)); return *this; }
        List& operator=(const List& other) noexcept { Copy(other); return *this; }

        inline void Copy(const T* elements, size_t count)
        {
            Clear();
            Reserve(count, false);
            Memory::CopyArray(GetData(), elements, count);
            m_count = count;
        }

        inline void Copy(const List& other)
        {
            Copy(other.GetData(), other.GetCount());
        }

        inline void Move(const T* elements, size_t count)
        {
            Reserve(count);
            Memory::MoveArray(GetData(), elements, count);
            m_count = count;
        }

        inline void Move(List&& other)
        {
            if (this != &other)
            {
                TData::Free(m_data);
                m_data = PK::Exchange(other.m_data, {});
                m_count = PK::Exchange(other.m_count, 0ull);
            }
        }

        bool Reserve(size_t newCapacity, bool preserve)
        {
            if (newCapacity > TData::GetCount(m_data))
            {
                auto newData = TData::Allocate(newCapacity);

                if (preserve && m_count > 0u)
                {
                    Memory::MoveArray(TData::GetPtr(newData), TData::GetPtr(m_data), m_count);
                }
                
                TData::Free(m_data);
                m_data = newData;
                return false;
            }

            return true;
        }

        void Resize(size_t count)
        {
            Reserve(count, true);

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

        void ClearFast()
        {
            m_count = 0u;
        }

        template<typename ... Args>
        T* Add(Args&& ... args)
        {
            Reserve(m_count + 1u, true);
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
        TData m_data;
        uint32_t m_count;
    };


    template<typename T, size_t capacity>
    using FixedArray = Array<T, AllocationFixed<capacity>>;

    template<typename T, size_t inline_capacity>
    using InlineArray = Array<T, AllocationInline<inline_capacity>>;

    template<typename T>
    using HeapArray = Array<T, AllocationHeap>;


    template<typename T, size_t capacity>
    using FixedList = List<T, AllocationFixed<capacity>>;

    template<typename T, size_t inline_capacity>
    using InlineList = List<T, AllocationInline<inline_capacity>>;

    template<typename T>
    using HeapList = List<T, AllocationHeap>;
}

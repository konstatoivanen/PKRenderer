#pragma once
#include "NoCopy.h"
#include "BufferView.h"
#include "BufferIterator.h"
#include <cstdint>
#include <exception>

namespace PK
{
    template<typename T, size_t inlineCapacity = 1u>
    struct FastList : NoCopy
    {
        FastList() {}
        FastList(size_t capacity) { Reserve(capacity); }
        FastList(FastList&& other) noexcept { Move(std::forward<FastBuffer>(other)); }
        FastList(const FastList& other) noexcept { Copy(other); }
        FastList(std::initializer_list<T>&& other) noexcept { Move(std::forward<std::initializer_list<T>>(other)); }
        FastList(const std::initializer_list<T>& other) noexcept { Copy(other); }

        ~FastList()
        {
            Clear();

            if (!IsSmallBuffer(m_capacity))
            {
                free(m_data.buffer);
            }

            m_capacity = 0u;
            m_data.buffer = nullptr;
        }

        T* GetData() { return reinterpret_cast<T*>(IsSmallBuffer(m_capacity) ? &m_data : m_data.buffer); }
        T const* GetData() const { return reinterpret_cast<const T*>(IsSmallBuffer(m_capacity) ? &m_data : m_data.buffer); }

        constexpr size_t GetCount() const { return m_count; }
        constexpr size_t GetSize() const { return m_count * sizeof(T); }

        BufferView<T> GetView() { return { GetData(), m_count }; }
        ConstBufferView<T> GetView() const { return { GetData(), m_count }; }

        ConstBufferIterator<T> begin() const { return ConstBufferIterator<T>(GetData(), 0ull); }
        ConstBufferIterator<T> end() const { return ConstBufferIterator<T>(GetData() + m_count, m_count); }

        T& operator [](size_t i) { return GetData()[i]; }
        T const& operator [](size_t i) const { return GetData()[i]; }

        operator T* () { return GetData(); }
        operator T const* () const { return GetData(); }

        FastList& operator=(FastList&& other) noexcept { Move(std::forward<FastBuffer>(other)); return *this; }

        void Copy(const FastList& other)
        {
            Clear();
            Reserve(other.m_count);
            std::copy(other.GetData(), other.GetData() + other.m_count, GetData());
        }

        void Copy(const std::initializer_list<T>& initializer)
        {
            Resize(initializer.size());
            std::copy(initializer.begin(), initializer.end(), GetData());
        }

        void Move(FastList&& other)
        {
            if (this != &other)
            {
                if (!IsSmallBuffer(m_capacity))
                {
                    free(m_data.buffer);
                }

                m_data = std::exchange(other.m_data, { nullptr });
                m_count = std::exchange(other.m_count, 0ull);
                m_capacity = std::exchange(other.m_capacity, 0ull);
            }
        }

        void Move(std::initializer_list<T>&& initializer)
        {
            Reserve(initializer.size());
            std::move(initializer.begin(), initializer.end(), GetData());
            m_count = initializer.size();
        }

        bool Reserve(size_t capacity)
        {
            if (capacity <= m_capacity)
            {
                return false;
            }

            auto buffer = reinterpret_cast<T*>(malloc(sizeof(T) * capacity));

            if (buffer == nullptr)
            {
                throw std::exception("Failed to allocate new buffer!");
            }
            
            if (m_count > 0u)
            {
                std::move(GetData(), GetData() + m_count, buffer);
            }

            if (!IsSmallBuffer(m_capacity))
            {
                free(m_data.buffer);
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
                new(GetData() + m_count++) T();
            }
        }

        void Clear() 
        {
            for (auto i = 0u; i < m_count; ++i)
            {
                (GetData() + i)->~T();
            }

            m_count = 0u;
        }

        template<typename ... Args>
        T* Add(Args&& ... args)
        {
            Reserve(m_count + 1u);
            return new(GetData() + m_count++) T(std::forward<Args>(args)...);
        }

        bool Remove(T* ptr)
        {
            return RemoveAt((uint32_t)(ptr - GetData()));
        }

        bool RemoveAt(uint32_t i)
        {
            if (i < m_count)
            {
                return false;
            }

            if (i == --m_count)
            {
                (GetData() + i)->~T();
                return true;
            }

            for (; i < m_count; ++i)
            {
                GetData()[i] = std::move(GetData()[i + 1]);
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
                (GetData() + i)->~T();
                return true;
            }

            if (m_count > 0u)
            {
                GetData()[i] = std::move(GetData()[m_count]);
                return true;
            }

            return false;
        }

    private:
        constexpr static bool IsSmallBuffer(size_t count) { return count <= (sizeof(U) / sizeof(T)); }
        struct U { union { void* buffer; alignas(T) unsigned char inl[sizeof(T) * inlineCapacity]; }; };
        U m_data{};
        uint32_t m_count = 0u;
        uint32_t m_capacity = inlineCapacity;
    };
}

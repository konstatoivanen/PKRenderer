#pragma once
#include "NoCopy.h"
#include "BufferView.h"
#include "BufferIterator.h"
#include <cstdint>
#include <exception>

namespace PK
{
    template<typename T>
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
            free(m_buffer);
            m_buffer = nullptr;
            m_capacity = 0u;
        }

        T* GetData() { return m_buffer; }
        T const* GetData() const { return m_buffer; }

        constexpr size_t GetCount() const { return m_count; }
        constexpr size_t GetSize() const { return m_count * sizeof(T); }

        BufferView<T> GetView() { return { m_buffer, m_count }; }
        ConstBufferView<T> GetView() const { return { m_buffer, m_count }; }

        ConstBufferIterator<T> begin() const { return ConstBufferIterator<T>(m_buffer, 0ull); }
        ConstBufferIterator<T> end() const { return ConstBufferIterator<T>(m_buffer + m_count, m_count); }

        T& operator [](size_t i) { return m_buffer[i]; }
        T const& operator [](size_t i) const { return m_buffer[i]; }

        operator T* () { return m_buffer; }
        operator T const* () const { return m_buffer; }

        FastList& operator=(FastList&& other) noexcept { Move(std::forward<FastBuffer>(other)); return *this; }

        void Copy(const FastList& other)
        {
            Clear();
            Reserve(other.m_count);



            std::copy(other.m_buffer, other.m_buffer + other.m_count, m_buffer);
        }

        void Copy(const std::initializer_list<T>& initializer)
        {
            Resize(initializer.size());
            std::copy(initializer.begin(), initializer.end(), m_buffer);
        }

        void Move(FastList&& other)
        {
            if (this != &other)
            {
                if (m_buffer)
                {
                    free(m_buffer);
                }

                m_buffer = std::exchange(other.m_buffer, nullptr);
                m_count = std::exchange(other.m_count, 0ull);
                m_capacity = std::exchange(other.m_capacity, 0ull);
            }
        }

        void Move(std::initializer_list<T>&& initializer)
        {
            Reserve(initializer.size());
            std::move(initializer.begin(), initializer.end(), m_buffer);
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
            
            if (m_buffer && m_count > 0u)
            {
                std::move(m_buffer, m_buffer + m_count, buffer);
            }

            free(m_buffer);
            m_buffer = buffer;
            m_capacity = capacity;
            return true;
        }

        void Resize(size_t count)
        {
            Reserve(count);

            while (m_count < count)
            {
                new(m_buffer + m_count++) T();
            }
        }

        void Clear() 
        {
            for (auto i = 0u; i < m_count; ++i)
            {
                (m_buffer + i)->~T();
            }

            m_count = 0u;
        }

        template<typename ... Args>
        T* Add(Args&& ... args)
        {
            if (m_count >= m_capacity)
            {
                Reserve(m_count + 1u);
            }

            return new(m_buffer + m_count++) T(std::forward<Args>(args)...);
        }

        bool Remove(T* ptr)
        {
            return RemoveAt((uint32_t)(ptr - m_buffer));
        }

        bool RemoveAt(uint32_t i)
        {
            if (i >= m_count)
            {
                return false;
            }

            if (i == --m_count)
            {
                (m_buffer + i)->~T();
                return true;
            }

            for (; i < m_count; ++i)
            {
                m_buffer[i] = std::move(m_buffer[i + 1]);
            }

            return true;
        }

    private:
        T* m_buffer = nullptr;
        uint32_t m_count = 0u;
        uint32_t m_capacity = 0u;
    };
}

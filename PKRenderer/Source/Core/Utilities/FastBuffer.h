#pragma once
#include "NoCopy.h"
#include "BufferView.h"
#include "BufferIterator.h"
#include <cstdint>
#include <exception>

namespace PK
{
    /// <summary>
    /// A non owning container. dont use for types that need implicit destructors.
    /// </summary>
    template<typename T, size_t inlineCapacity = 1u>
    struct FastBuffer : NoCopy
    {
        FastBuffer(size_t count) { Reserve(count); }
        FastBuffer() {}
        FastBuffer(FastBuffer&& other) noexcept { Move(std::forward<FastBuffer>(other)); }

        ~FastBuffer()
        {
            if (!IsSmallBuffer(m_count))
            {
                free(m_data.buffer);
                m_data.buffer = nullptr;
                m_count = 0u;
            }
        }

        T* GetData() { return reinterpret_cast<T*>(IsSmallBuffer(m_count) ? &m_data : m_data.buffer); }
        T const* GetData() const { return reinterpret_cast<const T*>(IsSmallBuffer(m_count) ? &m_data : m_data.buffer); }

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

        FastBuffer& operator=(FastBuffer&& other) noexcept { Move(std::forward<FastBuffer>(other)); return *this; }

        void Copy(const FastBuffer& other)
        {
            Reserve(other.m_count);
            std::copy(other.GetData(), other.GetData() + other.m_count, GetData());
        }

        void Copy(const std::initializer_list<T>& initializer)
        {
            Reserve(initializer.size());
            std::copy(initializer.begin(), initializer.end(), GetData());
        }

        void Move(FastBuffer&& other)
        {
            if (this != &other)
            {
                if (!IsSmallBuffer(m_count))
                {
                    free(m_data.buffer);
                }

                m_data = std::exchange(other.m_data, { nullptr });
                m_count = std::exchange(other.m_count, 0ull);
            }
        }

        bool Reserve(size_t count)
        {
            if (count <= m_count)
            {
                return false;
            }
            
            if (IsSmallBuffer(count))
            {
                m_count = count;
                return false;
            }

            auto oldSize = sizeof(T) * m_count;
            auto newSize = sizeof(T) * count;
            auto newbuffer = !IsSmallBuffer(m_count) ? realloc(m_data.buffer, newSize) : calloc(count, sizeof(T));

            if (newbuffer == nullptr)
            {
                throw std::exception("Failed to allocate new buffer!");
            }
            
            if (m_count > 0 && IsSmallBuffer(m_count))
            {
                memcpy(newbuffer, &m_data, oldSize);
            }
            else if (m_count > 0)
            {
                // Realloc doesnt zero memory
                memset(reinterpret_cast<char*>(newbuffer) + oldSize, 0, newSize - oldSize);
            }

            m_data.buffer = newbuffer;
            m_count = count;
            return true;
        }

        void Clear() { memset(GetData(), 0, sizeof(T) * m_count); }

    private:
        constexpr static bool IsSmallBuffer(size_t count) { return count <= (sizeof(U) / sizeof(T)); }
        
        struct U { union { void* buffer; alignas(T) unsigned char inl[sizeof(T) * inlineCapacity]; }; };
        U m_data{};
        size_t m_count = 0ull;
    };
}

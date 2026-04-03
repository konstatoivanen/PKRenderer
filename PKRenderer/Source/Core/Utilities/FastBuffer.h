#pragma once
#include "NoCopy.h"
#include "BufferView.h"
#include "Memory.h"

namespace PK
{
    /// <summary>
    /// A non owning container. dont use for types that need implicit destructors.
    /// </summary>
    template<typename T, size_t fixed_count = 0ull>
    struct FastBuffer : NoCopy
    {
        FastBuffer(size_t count) { Reserve(count); }
        FastBuffer() {}
        FastBuffer(FastBuffer&& other) noexcept { Move(PK::Forward<FastBuffer>(other)); }

        ~FastBuffer()
        {
            if (!IsSmallBuffer(m_count))
            {
                Memory::Free(m_data.buffer);
                m_data.buffer = nullptr;
                m_count = 0u;
            }
        }

        T* GetData() { return IsSmallBuffer(m_count) ? reinterpret_cast<T*>(&m_data) : m_data.buffer; }
        T const* GetData() const { return IsSmallBuffer(m_count) ? reinterpret_cast<T const*>(&m_data) : m_data.buffer; }

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

        FastBuffer& operator=(FastBuffer&& other) noexcept { Move(PK::Forward<FastBuffer>(other)); return *this; }

        void Copy(const FastBuffer& other)
        {
            Reserve(other.m_count);
            Memory::CopyArray(GetData(), other.GetData(), other.m_count);
        }

        void Copy(const std::initializer_list<T>& initializer)
        {
            Reserve(initializer.size());
            Memory::CopyArray(GetData(), initializer.begin(), initializer.size());
        }

        void Move(FastBuffer&& other)
        {
            if (this != &other)
            {
                if (!IsSmallBuffer(m_count))
                {
                    Memory::Free(m_data.buffer);
                }

                m_data = PK::Exchange(other.m_data, { nullptr });
                m_count = PK::Exchange(other.m_count, 0ull);
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

            auto buffer = Memory::ReallocOrCalloc<T>(m_data.buffer, count, IsSmallBuffer(m_count));

            if (m_count > 0 && IsSmallBuffer(m_count))
            {
                Memory::Memcpy<T>(buffer, reinterpret_cast<T*>(&m_data), m_count);
            }
            else if (m_count > 0)
            {
                // Realloc doesnt zero memory
                Memory::Memset<T>(buffer + m_count, 0, count - m_count);
            }

            m_data.buffer = buffer;
            m_count = count;
            return true;
        }

        void Clear() { memset(GetData(), 0, sizeof(T) * m_count); }

    private:
        constexpr static bool IsSmallBuffer(size_t count) { return count <= (sizeof(U) / sizeof(T)); }
        
        struct U { union { T* buffer; alignas(T) uint8_t inl[fixed_count > 0ull ? sizeof(T) * fixed_count : sizeof(T*)]; }; };
        U m_data{};
        size_t m_count = 0ull;
    };
}

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
    class MemoryBlock : NoCopy
    {
        struct Data
        {
            union
            {
                void* memory;
                char inlineMemory[sizeof(T) * inlineCapacity];
            };
        };

        constexpr static size_t MaxSmallBufferCount() { return sizeof(Data) / sizeof(T); }
        constexpr static bool IsSmallBuffer(size_t count) { return count <= MaxSmallBufferCount(); }

        public:
            MemoryBlock(size_t count) { Validate(count); }
            MemoryBlock() {}
            
            ~MemoryBlock()
            {
                if (m_data.memory != nullptr && !IsSmallBuffer(m_count))
                {
                    free(m_data.memory);
                }
            }

            void Validate(size_t count)
            {
                if (count <= m_count)
                {
                    return;
                }
                
                if (IsSmallBuffer(count))
                {
                    m_count = count;
                    return;
                }

                auto oldSize = sizeof(T) * m_count;
                auto newSize = sizeof(T) * count;
                auto newbuffer = !IsSmallBuffer(m_count) ? realloc(m_data.memory, newSize) : calloc(count, sizeof(T));

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

                m_data.memory = newbuffer;
                m_count = count;
            }

            void CopyFrom(const MemoryBlock& other)
            {
                Validate(other.m_count);
                std::copy(other.GetData(), other.GetData() + other.m_count, GetData());
            }

            void CopyFrom(const std::initializer_list<T>& initializer)
            {
                Validate(initializer.size());
                std::copy(initializer.begin(), initializer.end(), GetData());
            }

            void Clear() 
            {
                memset(IsSmallBuffer(m_count) ? &m_data : m_data.memory, 0, sizeof(T) * m_count);
            }

            T* GetData() { return reinterpret_cast<T*>(IsSmallBuffer(m_count) ? &m_data : m_data.memory); }
            T const* GetData() const { return reinterpret_cast<const T*>(IsSmallBuffer(m_count) ? &m_data : m_data.memory); }

            BufferView<T> GetView() { return { GetData(), m_count }; }
            ConstBufferView<T> GetView() const { return { GetData(), m_count }; }
            
            ConstBufferIterator<T> begin() const { return ConstBufferIterator<T>(GetData(), 0ull); }
            ConstBufferIterator<T> end() const { return ConstBufferIterator<T>(GetData() + m_count, m_count); }

            T& operator [](size_t i) { return GetData()[i]; }
            T const& operator [](size_t i) const { return GetData()[i]; }

            operator T* () { return GetData(); }
            operator T const* () const { return GetData(); }

            constexpr size_t GetCount() const { return m_count; }
            constexpr size_t GetSize() const { return m_count * sizeof(T); }

        private:
            Data m_data = { nullptr };
            size_t m_count = 0ull;
    };
}

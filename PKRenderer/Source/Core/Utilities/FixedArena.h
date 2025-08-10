#pragma once
#include "NoCopy.h"
#include "ContainerHelpers.h"
#include <exception>

namespace PK
{
    template<size_t capacity>
    struct FixedArena : public NoCopy
    {
        FixedArena()
        {
        }

        template<typename T>
        T* Allocate(size_t count)
        {
            static_assert(std::is_trivially_copyable_v<T>, "This container only supports trivially copyable types.");
            
            if (count == 0u)
            {
                return nullptr;
            }

            const auto alignment = alignof(T);
            const auto headAddress = reinterpret_cast<uint64_t>(m_data + m_head);
            const auto headAligned = ((headAddress + alignment - 1ull) & ~(alignment - 1ull)) - reinterpret_cast<uint64_t>(m_data);
            
            m_head = headAligned + sizeof(T) * count;
            PK_CONTAINER_RANGE_CHECK(m_head, 0ull, capacity);
            return reinterpret_cast<T*>(m_data + headAligned);
        }

        void Clear() 
        { 
            m_head = 0ull;
            memset(m_data, 0, sizeof(char) * capacity); 
        }

    private:
        char m_data[capacity]{};
        size_t m_head = 0ull;
    };
}
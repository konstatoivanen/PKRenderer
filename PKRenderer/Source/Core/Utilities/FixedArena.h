#pragma once
#include "NoCopy.h"
#include "ContainerHelpers.h"
#include <exception>

namespace PK
{
    struct IArena : public NoCopy
    {
        template<typename T>
        T* GetHead() { return reinterpret_cast<T*>(GetAlignedHead(alignof(T))); }

        template<typename T>
        size_t GetHeadDelta(const T* element) const { return (size_t)(reinterpret_cast<const T*>(GetAlignedHead(alignof(T))) - element); }

        template<typename T>
        T* Allocate(size_t count)
        {
            static_assert(std::is_trivially_copyable_v<T>, "This container only supports trivially copyable types.");
            return count > 0ull ? reinterpret_cast<T*>(AllocateBlock(sizeof(T) * count, alignof(T))) : nullptr;
        }

        // Warning destructor must be manually called for retrieved pointer.
        template<typename T, typename ... Args>
        T* New(Args&& ... args)
        {
            auto ptr = reinterpret_cast<T*>(AllocateBlock(sizeof(T), alignof(T)));
            new(ptr) T(std::forward<Args>(args)...);
            return ptr;
        }

        template<typename T>
        T* Emplace(T&& element)
        {
            return New<T>(std::forward<T>(element));
        }

        virtual uint64_t GetAlignedHead(size_t alignment) const = 0;
        virtual uint64_t GetRelativeHead(size_t alignment) const = 0;
        virtual void* AllocateBlock(size_t size, size_t alignment) = 0;
        virtual void Clear() = 0;
        virtual void ClearFast() = 0;
     };

    template<size_t capacity>
    struct FixedArena : public IArena
    {
        FixedArena()
        {
        }

        uint64_t GetAlignedHead(size_t alignment) const final { return ((reinterpret_cast<uint64_t>(m_data + m_head) + alignment - 1ull) & ~(alignment - 1ull)); }
        uint64_t GetRelativeHead(size_t alignment) const final { return GetAlignedHead(alignment) - reinterpret_cast<uint64_t>(m_data); }

        void* AllocateBlock(size_t size, size_t alignment) final
        {
            auto relativeHead = GetRelativeHead(alignment);
            m_head = relativeHead + size;
            PK_CONTAINER_RANGE_CHECK(m_head, 0ull, capacity);
            return m_data + relativeHead;
        }

        void Clear() final
        { 
            m_head = 0ull;
            memset(m_data, 0, sizeof(char) * capacity); 
        }

        void ClearFast() final
        {
            m_head = 0ull;
        }

        uint8_t m_data[capacity]{};
        size_t m_head = 0ull;
    };
}

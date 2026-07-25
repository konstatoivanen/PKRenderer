#pragma once
#include "IEntityImplementer.h"

namespace PK
{
    // Note that reference count access isnt protected against null data.
    // Such cases are erroneous and we would ideally crash anyway.
    template<typename T>
    struct EntityComponentRef
    {
        using Type = T;
        IEntityImplementer* shared = nullptr;
        T* pointer = nullptr;

        constexpr EntityComponentRef() = default;

        EntityComponentRef(const EntityComponentRef& other) noexcept
        {
            shared = other.shared;
            pointer = other.pointer;
            shared->referenceCount++;
        }

        ~EntityComponentRef() 
        {
            if (shared && pointer && --shared->referenceCount == 0u)
            {
                shared->bucket->destroyAt(shared->bucket, shared->index);
                pointer = nullptr;
                shared = nullptr;
            }
        }

        EntityComponentRef& operator=(const EntityComponentRef& other) noexcept
        {
            shared = other.shared;
            pointer = other.pointer;
            shared->referenceCount++;
            return *this;
        }

        template<typename TImpl>
        EntityComponentRef& operator=(TImpl* other) noexcept
        { 
            shared = other;
            pointer = static_cast<T*>(other);
            shared->referenceCount++;
            return *this;
        }


        T* operator*() const noexcept { return pointer; }
        T* operator->() const noexcept { return pointer; }
        explicit operator bool() const noexcept { return pointer != nullptr; }
    };
}

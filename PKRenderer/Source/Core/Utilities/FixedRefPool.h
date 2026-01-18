#pragma once
#include "NoCopy.h"
#include "FixedPool.h"
#include "Ref.h"

namespace PK
{
    template<typename T>
    struct PooledSharedObject : public SharedObjectBase
    {
        template <typename ... Args>
        explicit PooledSharedObject(Args&&... args) : SharedObjectBase() { new(&value) T(std::forward<Args>(args)...); }
        virtual ~PooledSharedObject() noexcept override { Destroy(); }

        void Destroy() noexcept final 
        {
            if (isConstructed)
            {
                value.~T(); 
                isConstructed = 0u; 
            }
        }

        void Delete() noexcept final 
        { 
            pool->Delete(this);
        }

        struct U { constexpr U() noexcept {} };
        union { U unionDefault; T value; };
        IPool<PooledSharedObject<T>>* pool = nullptr;
        bool isConstructed = true;
    };

    template<typename T, size_t capacity>
    struct FixedRefPool : protected FixedPool<PooledSharedObject<T>, capacity>
    {
        using TShared = PooledSharedObject<T>;
        using TBase = FixedPool<PooledSharedObject<T>, capacity>;
        using TBase::GetCount;
        using TBase::GetCapacity;

        FixedRefPool() : TBase() {};

        template<typename ... Args>
        Ref<T> CreateRef(Args&& ... args)
        {
            auto sharedObject = TBase::New(std::forward<Args>(args)...);
            sharedObject->pool = this;
            return CreateRefAliased<T, TShared>(sharedObject);
        }
    };
}

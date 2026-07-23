#pragma once
#include "Mask.h"
#include "Ref.h"

// MSVC adds an extra 16 bytes of padding to the members here for some reason.
// Even though both members are size aligned to 16 bytes... ????
#pragma warning( push )
#pragma warning( disable : 4324 )

namespace PK
{
    template<typename T>
    struct IPool : public NoCopy
    {
        using Type = T;

        T* operator [](uint32_t index) { return GetData() + index; }
        const T* operator [](uint32_t index) const { return GetData() + index; }

        virtual T* GetData() = 0;
        virtual const T* GetData() const = 0;
        virtual size_t GetCount() const = 0;
        virtual size_t GetCapacity() const = 0;

        virtual void Delete(const T* ptr, size_t count = 1ull) = 0;
        virtual void Clear() = 0;

        uint32_t GetIndex(const T* ptr) const
        {
            Memory::Assert(ptr >= GetData() && ptr < GetData() + GetCapacity(), "Ptr is outiside of pool bounds!");
            return (uint32_t)(ptr - GetData());
        }

        template<typename ... Args>
        T* New(Args&& ... args) { return Memory::Construct(Allocate(1u), PK::Forward<Args>(args)...); }

        template<typename ... Args>
        T* NewAt(int64_t index, Args&& ... args) { return Memory::Construct(Allocate(1u, index), PK::Forward<Args>(args)...); }

        template<typename ... Args>
        T* NewArray(size_t count, Args&& ... args) { return Memory::ConstructArray(Allocate(count), count, PK::Forward<Args>(args)...); }

        void Delete(uint32_t index) { Delete(GetData() + index); }
        
        protected:
            virtual T* Allocate(size_t count, int64_t index = -1) = 0;
    };

    template<typename T, typename TAllocation, typename TMask>
    struct Pool : public IPool<T>
    {
        using TData = typename TAllocation::template Data<T>;

        using IPool<T>::New;
        using IPool<T>::NewAt;
        using IPool<T>::GetIndex;
        using IPool<T>::Delete;

        Pool() : m_data(), m_mask() {}
        Pool(size_t capacity) noexcept : Pool() { Reserve(capacity); }
        Pool(Pool&& other) noexcept : Pool() { Move(PK::Forward<Pool>(other)); }
        Pool(const Pool& other) noexcept : Pool() { Copy(other); }
        ~Pool() { Clear(); TData::Free(m_data); }

        Pool& operator=(Pool&& other) noexcept { Move(PK::Forward<Pool>(other)); return *this; }
        Pool& operator=(const Pool& other) noexcept { Copy(other); return *this; }

        T* GetData() final { return TData::GetPtr(m_data); }
        const T* GetData() const final { return TData::GetPtr(m_data); }
        size_t GetCount() const final { return m_mask.CountBits(); }
        size_t GetCapacity() const final { return TData::GetCount(m_data); }
        const TMask& GetActiveMask() { return m_mask; }

        inline void Copy(const Pool& other)
        {
            Clear();
            Reserve(other.GetCapacity());

            for (auto bit : other.m_mask)
            {
                if (bit)
                {
                    GetData()[bit.index()] = other.GetData()[bit.index()];
                    m_mask.SetAt(bit.index(), true);
                }
            }
        }

        inline void Move(Pool&& other)
        {
            if (this != &other)
            {
                TData::Free(m_data);
                m_mask.Move(other.m_mask);
                m_data = PK::Exchange(other.m_data, {});
            }
        }

        bool Reserve(size_t newCapacity, bool preserve)
        {
            if (newCapacity > GetCapacity())
            {
                m_mask.Reserve(newCapacity, preserve);

                auto newData = TData::Allocate(newCapacity);

                if (preserve && GetCapacity() > 0u)
                {
                    for (auto bit : m_mask)
                    {
                        if (bit)
                        {
                            TData::GetPtr(newData)[bit.index()] = PK::MoveTemp(GetData()[bit.index()]);
                        }
                    }
                }

                TData::Free(m_data);
                m_data = newData;
                return true;
            }

            return false;
        }

        void Delete(const T* ptr, size_t count = 1ull) final
        {
            auto index = GetIndex(ptr);

            for (auto i = 0u; i < count; ++i)
            {
                if (m_mask.GetAt(index + i))
                {
                    m_mask.FlipAt(index + i);
                    Memory::Destruct(ptr);
                }
            }
        }

        void Delete(const TMask& mask)
        {
            for (auto& bit : m_mask)
            {
                if (bit)
                {
                    Delete(bit.index());
                }
            }
        }

        void Clear() final
        {
            for (auto& bit : m_mask)
            {
                if (bit)
                {
                    Memory::Destruct(GetData() + bit.index());
                    m_mask.FlipAt(bit.index());
                }
            }
        }

    protected:
        T* Allocate(size_t count, int64_t index) final
        {
            index = index != -1ll ? index : m_mask.FindFirstZeroRange((uint32_t)count);
            Memory::Assert(index >= 0ll && index + count - 1ll < GetCapacity(), "Pool capacity exceeded!");
            auto ptr = GetData() + index;
            m_mask.FlipRange(index, index + count);
            return ptr;
        }

        TData m_data;
        TMask m_mask;
    };

    template<typename T>
    struct PooledSharedObject : public SharedObjectBase
    {
        template <typename ... Args>
        explicit PooledSharedObject(Args&&... args) : SharedObjectBase() { Memory::Construct(&value, PK::Forward<Args>(args)...); }
        virtual ~PooledSharedObject() noexcept override { Destroy(); }

        void Destroy() noexcept final
        {
            if (isConstructed)
            {
                Memory::Destruct(&value);
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

    template<typename T, typename TBase>
    struct RefPool : protected TBase
    {
        using TShared = PooledSharedObject<T>;
        using TBase::GetCount;
        using TBase::GetCapacity;

        RefPool() : TBase() {};

        template<typename ... Args>
        Ref<T> CreateRef(Args&& ... args)
        {
            auto sharedObject = TBase::New(PK::Forward<Args>(args)...);
            sharedObject->pool = this;
            return CreateRefAliased<T, TShared>(sharedObject);
        }
    };


    template<typename T, size_t capacity>
    using FixedPool = Pool<T, AllocationFixed<capacity>, FixedMask<capacity>>;

    template<typename T, size_t inline_capacity>
    using InlinePool = Pool<T, AllocationInline<inline_capacity>, InlineMask<inline_capacity>>;

    template<typename T>
    using HeapPool = Pool<T, AllocationHeap, HeapMask>;


    template<typename T, size_t capacity>
    using FixedRefPool = RefPool<T, FixedPool<PooledSharedObject<T>, capacity>>;

    template<typename T, size_t inline_capacity>
    using InlineRefPool = RefPool<T, InlinePool<PooledSharedObject<T>, inline_capacity>>;

    template<typename T>
    using HeapRefPool = RefPool<T, HeapPool<PooledSharedObject<T>>>;
}

#pragma warning( pop )

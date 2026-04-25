#pragma once
#include "NoCopy.h"
#include "FixedMask.h"
#include "Memory.h"

namespace PK
{
    template<typename T>
    struct IPool : public NoCopy
    {
        using Type = T;

        virtual T* GetData() = 0;
        virtual const T* GetData() const = 0;
        virtual size_t GetCount() const = 0;
        virtual size_t GetCapacity() const = 0;

        virtual void Delete(const T* ptr, size_t count = 1ull) = 0;
        virtual void Clear() = 0;

        T* operator [](uint32_t index) { return GetData() + index; }
        const T* operator [](uint32_t index) const { return GetData() + index; }

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

    template<typename T, size_t capacity>
    struct FixedPool : public IPool<T>
    {
        using IPool<T>::New;
        using IPool<T>::NewAt;
        using IPool<T>::GetIndex;
        using IPool<T>::Delete;

        FixedPool() : m_mask() {}
        ~FixedPool() { Clear(); }

        const T* GetData() const final { return reinterpret_cast<const T*>(m_data); }
        T* GetData() final { return reinterpret_cast<T*>(m_data); }
        size_t GetCount() const final { return m_mask.CountBits(); }
        size_t GetCapacity() const final { return capacity; }
        const FixedMask<capacity>& GetActiveMask() { return m_mask; }

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

        void Delete(const FixedMask<capacity>& mask)
        {
            for (auto i = 0u; i < mask.Size; ++i)
            {
                for (uint32_t j = i * mask.Stride, k = j + mask.Stride; j < k && mask.m_mask[i] != 0ull; ++j)
                {
                    if (mask.GetAt(j))
                    {
                        Delete(j);
                    }
                }
            }
        }

        void Clear() final
        {
            const auto hasAny = m_mask.CountBits();

            for (auto i = 0u; i < capacity && hasAny; ++i)
            {
                if (m_mask.GetAt(i))
                {
                    Memory::Destruct(GetData() + i);
                    m_mask.FlipAt(i);
                }
            }
        }

        protected:
            T* Allocate(size_t count, int64_t index) final
            {
                index = index != -1ll ? index : m_mask.FindFirstZeroRange((uint32_t)count);
                Memory::Assert(index >= 0ll && index + count - 1ll < capacity, "Pool capacity exceeded!");
                auto ptr = GetData() + index;
                m_mask.FlipRange(index, index + count);
                return ptr;
            }

        private:
            alignas(T) uint8_t m_data[sizeof(T) * capacity];
            FixedMask<capacity> m_mask;
    };
}

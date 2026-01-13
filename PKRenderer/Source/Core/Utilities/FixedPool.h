#pragma once
#include "NoCopy.h"
#include "FixedMask.h"
#include "ContainerHelpers.h"

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

        virtual void Delete(const T* ptr) = 0;
        virtual void Clear() = 0;

        T* operator [](uint32_t index) { return GetData() + index; }
        const T* operator [](uint32_t index) const { return GetData() + index; }

        uint32_t GetIndex(const T* ptr) const
        {
            PK_CONTAINER_RANGE_CHECK(ptr, GetData(), (GetData() + GetCapacity()));
            return (uint32_t)(ptr - GetData());
        }

        template<typename ... Args>
        T* New(Args&& ... args)
        {
            auto ptr = Allocate();
            new(ptr) T(std::forward<Args>(args)...);
            return ptr;
        }

        template<typename ... Args>
        T* NewAt(int64_t index, Args&& ... args)
        {
            auto ptr = Allocate(index);
            new(ptr) T(std::forward<Args>(args)...);
            return ptr;
        }

        void Delete(uint32_t index)
        {
            Delete(GetData() + index);
        }
        
        protected:
            virtual T* Allocate(int64_t index = -1) = 0;
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

        void Delete(const T* ptr) final
        {
            auto index = GetIndex(ptr);
            if (m_mask.GetAt(index))
            {
                m_mask.ToggleAt(index);
                ptr->~T();
            }
        }

        void Delete(const FixedMask<capacity>& mask)
        {
            // if a whole 64 bit segment is empty we can assume that the rest are as well.
            // Could lead to issues in a very unlucky scenario.
            for (auto i = 0u; i < mask.Size && mask.m_mask[i] != 0ull; ++i)
            {
                for (uint32_t j = i * mask.Stride, k = j + mask.Stride; j < k; ++j)
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
            for (auto i = 0u; i < capacity; ++i)
            {
                if (m_mask.GetAt(i))
                {
                    (GetData() + i)->~T();
                    m_mask.ToggleAt(i);
                }
            }
        }

        protected:
            T* Allocate(int64_t index) final
            {
                index = index != -1 ? index : m_mask.FirstFalse();
                PK_CONTAINER_RANGE_CHECK(index, 0, (int64_t)capacity);
                auto ptr = GetData() + index;
                m_mask.ToggleAt(index);
                return ptr;
            }

        private:
            alignas(T) std::byte m_data[sizeof(T) * capacity];
            FixedMask<capacity> m_mask;
    };
}

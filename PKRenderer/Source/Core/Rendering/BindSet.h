#pragma once
#include "Core/Utilities/FastMap.h"
#include "Core/RHI/RHI.h"

namespace PK
{
    template<typename T>
    struct BindSet : public NoCopy
    {
        BindSet(uint32_t capacity) : m_array(RHI::CreateBindArray<T>(capacity)), m_indices(capacity, 1ull) {}

        uint32_t Set(T* value)
        {
            uint32_t setidx = 0u;
            if (!m_indices.Add(value, &setidx))
            {
                return (uint32_t)setidx;
            }

            auto arrayidx = m_array->Add(value);

            if ((uint32_t)arrayidx != setidx)
            {
                throw std::exception("Indexing missmatch!");
            }

            return arrayidx;
        }

        const T* Get(uint32_t index) const
        {
            return m_indices[index];
        }

        void Clear()
        {
            m_indices.Clear();
            m_array->Clear();
        }

        operator RHIBindArray<T>* () { return m_array.get(); }
        operator const RHIBindArray<T>* () { return m_array.get(); }

    private:
        RHIBindArrayRef<T> m_array;
        PointerSet<T> m_indices;
    };
}
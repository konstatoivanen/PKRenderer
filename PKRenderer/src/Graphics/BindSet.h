#pragma once
#include "Utilities/FastSet.h"
#include "Graphics/RHI/RHIBindArray.h"
#include "Graphics/RHI/RHI.h"

namespace PK::Graphics
{
    template<typename T>
    struct BindSet : public Utilities::NoCopy
    {
        BindSet(uint32_t capacity) : m_array(RHI::RHICreateBindArray<T>(capacity)), m_indices(capacity) {}

        uint32_t Set(T* value)
        {
            uint32_t setidx = 0u;

            if (!m_indices.Add(value, setidx))
            {
                return (uint32_t)setidx;
            }

            auto arrayidx = m_array->Add(value);

            if (arrayidx != setidx)
            {
                throw std::exception("Indexing missmatch!");
            }

            return arrayidx;
        }

        void Clear()
        {
            m_indices.Clear();
            m_array->Clear();
        }

        operator RHI::RHIBindArray<T>* () { return m_array.get(); }
        operator const RHI::RHIBindArray<T>* () { return m_array.get(); }

    private:
        RHI::RHIBindArrayRef<T> m_array;
        Utilities::PointerSet<T> m_indices;
    };
}
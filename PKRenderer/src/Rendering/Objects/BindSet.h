#pragma once
#include "BindArray.h"
#include "Utilities/IndexedSet.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Core;
    using namespace PK::Utilities;

    template<typename T>
    class BindSet : public NoCopy
    {
        public:
            BindSet(uint32_t capacity) : m_array(BindArray<T>::Create(capacity)), m_indices(capacity) {}

            uint32_t Set(T* value)
            {
                uint32_t setidx = 0u;
                
                if (!m_indices.Add(value, &setidx))
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

            operator BindArray<T>* () { return m_array.get(); }
            operator const BindArray<T>* () { return m_array.get(); }

        private:
            Ref<BindArray<T>> m_array;
            IndexedSet<T> m_indices;
    };
}
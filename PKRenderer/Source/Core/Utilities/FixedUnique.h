#pragma once
#include "NoCopy.h"
#include <exception>

namespace PK
{
    template<typename T>
    struct FixedUnique : public NoCopy
    {
        FixedUnique() : m_isCreated(false) {}

        ~FixedUnique()
        {
            Delete();
        }

        template<typename ... Args>
        void New(Args&& ... args)
        {
            if (!m_isCreated)
            {
                new(reinterpret_cast<T*>(m_data)) T(std::forward<Args>(args)...);
                m_isCreated = true;
            }
        }

        void Delete()
        {
            if (m_isCreated)
            {
                reinterpret_cast<T*>(m_data)->~T();
                m_isCreated = false;
            }
        }

        const T* get() const { return m_isCreated ? reinterpret_cast<const T*>(m_data) : nullptr; }
        T* get() { return m_isCreated ? reinterpret_cast<T*>(m_data) : nullptr; }

        operator const T* () const { return get(); }
        operator T* () { return get(); }

        const T* operator -> () const { return get(); }
        T* operator -> () { return get(); }

    private:
        bool m_isCreated;
        alignas(T) std::byte m_data[sizeof(T)]{};
    };
}

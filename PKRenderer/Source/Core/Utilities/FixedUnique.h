#pragma once
#include "NoCopy.h"

namespace PK
{
    template<typename T>
    struct FixedUnique : public NoCopy
    {
        FixedUnique() : m_isCreated(false) {}
        ~FixedUnique() { Delete(); }

        const T* get() const { return m_isCreated ? &value : nullptr; }
        T* get() { return m_isCreated ? &value : nullptr; }

        operator const T* () const { return get(); }
        operator T* () { return get(); }

        const T* operator -> () const { return get(); }
        T* operator -> () { return get(); }

        template<typename ... Args>
        void New(Args&& ... args)
        {
            if (!m_isCreated)
            {
                new(&value) T(std::forward<Args>(args)...);
                m_isCreated = true;
            }
        }

        void Delete()
        {
            if (m_isCreated)
            {
                reinterpret_cast<T*>(&value)->~T();
                m_isCreated = false;
            }
        }

    private:
        bool m_isCreated;
        struct U { constexpr U() noexcept {} };
        union { U other; T value; };
    };
}

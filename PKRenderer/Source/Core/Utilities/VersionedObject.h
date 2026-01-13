#pragma once
#include <cstdint>
#include "NoCopy.h"

namespace PK
{
    struct VersionedObject : public NoCopy
    {
        VersionedObject() : m_version(++s_globalVersion) {}
        virtual ~VersionedObject() = 0;

        constexpr uint64_t Version() const { return m_version; }
        inline void IncrementVersion() { m_version = ++s_globalVersion; }

        private:
            uint64_t m_version;
            inline static uint64_t s_globalVersion = 0ull;
    };

    template<typename T>
    struct VersionHandle
    {
        static_assert(std::is_base_of<VersionedObject, T>::value, "Template argument type does not derive from VersionedObject!");

        struct Hash
        {
            std::size_t operator()(const VersionHandle& k) const noexcept
            {
                return k.version;
            }
        };

        constexpr VersionHandle() : value(nullptr), version(0ull) {}
        constexpr VersionHandle(const T* value) : value(value), version(value ? value->Version() : 0ull) {}

        constexpr bool operator == (const VersionHandle& r) const noexcept { return value == r.value && version == r.version; }
        constexpr operator const T* () { return value; }
        constexpr operator bool () const { return value != nullptr; }
        const T* operator->() const { return value; }

        const T* value;
        uint64_t version;
    };
}

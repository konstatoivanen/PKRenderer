#pragma once
#include <cstdint>
#include "NoCopy.h"

namespace PK::Utilities
{
    struct IDObject : public NoCopy
    {
        IDObject() : m_version(++s_globalVersion) {}
        virtual ~IDObject() = 0 {};

        constexpr uint64_t Version() const { return m_version; }
        inline void IncrementVersion() { m_version = ++s_globalVersion; }

        private:
            uint64_t m_version;
            inline static uint64_t s_globalVersion = 0ull;
    };

    template<typename T>
    struct IDHandle
    {
        const T* value;
        uint64_t version;

        static_assert(std::is_base_of<IDObject, T>::value, "Template argument type does not derive from IDObject!");

        struct Hash
        {
            std::size_t operator()(const IDHandle& k) const noexcept
            {
                return k.version;
            }
        };

        constexpr IDHandle() : value(nullptr), version(0ull) {}
        constexpr IDHandle(const T* value) : value(value), version(value->Version()) {}

        constexpr bool operator == (const IDHandle& r) const noexcept
        {
            return value == r.value && version == r.version;
        }

        constexpr operator const T* () { return value; }

        const T* operator->() const { return value; }
    };
}
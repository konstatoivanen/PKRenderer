#pragma once
#include <cstdint>
#include "FixedString.h"

namespace PK
{
    struct INameIDProvider
    {
        virtual ~INameIDProvider() = 0;
        virtual uint32_t INameIDProvider_StringToID(const char* name) = 0;
        virtual const char* INameIDProvider_IDToString(const uint32_t& name) = 0;
    };

    struct NameID
    {
        uint32_t identifier = 0u;

        NameID() {}
        NameID(const char* name) : identifier(s_Provider->INameIDProvider_StringToID(name)) {}
        constexpr NameID(const NameID& name) : identifier(name.identifier) {}
        constexpr NameID(uint32_t identifier) : identifier(identifier) {}

        const char* c_str() const { return s_Provider->INameIDProvider_IDToString(identifier); }
        constexpr operator const uint32_t() const { return identifier; }

        static void SetProvider(INameIDProvider* provider) { s_Provider = provider; }

        private: inline static INameIDProvider* s_Provider;
    };

    constexpr static bool operator == (const NameID& a, const NameID& b) { return a.identifier == b.identifier; }
    constexpr static bool operator != (const NameID& a, const NameID& b) { return !(a == b); }
    constexpr static bool operator == (const NameID& a, const uint32_t& b) { return a.identifier == b; }
    constexpr static bool operator != (const NameID& a, const uint32_t& b) { return !(a == b); }
    constexpr static bool operator == (const uint32_t& a, const NameID& b) { return a == b.identifier; }
    constexpr static bool operator != (const uint32_t& a, const NameID& b) { return !(a == b); }
}

template <>
struct std::hash<PK::NameID>
{
    std::size_t operator()(const PK::NameID& k) const { return k.identifier; }
};

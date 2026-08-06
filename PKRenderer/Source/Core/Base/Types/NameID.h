#pragma once
#include "Core/Base/Hash.h"

namespace PK
{
    struct NameIDProvider;

    struct NameID
    {
        uint32_t identifier = 0u;

        constexpr NameID() = default;
        inline NameID(const char* name) : identifier(NameIDProvider_StringToID(name)) {}
        constexpr NameID(const NameID& name) : identifier(name.identifier) {}
        constexpr NameID(uint32_t identifier) : identifier(identifier) {}
        
        constexpr NameID& operator=(const NameID&) = default;
        constexpr NameID& operator=(NameID&&) = default;
        constexpr operator const uint32_t() const { return identifier; }

        inline const char* c_str() const { return NameIDProvider_IDToString(identifier); }

        static uint32_t NameIDProvider_StringToID(const char* name);
        static const char* NameIDProvider_IDToString(const uint32_t& name);
        static void SetProvider(NameIDProvider* provider) { s_Provider = provider; }
        private: inline static NameIDProvider* s_Provider;
    };

    constexpr static bool operator == (const NameID& a, const NameID& b) { return a.identifier == b.identifier; }
    constexpr static bool operator != (const NameID& a, const NameID& b) { return !(a == b); }
    constexpr static bool operator == (const NameID& a, const uint32_t& b) { return a.identifier == b; }
    constexpr static bool operator != (const NameID& a, const uint32_t& b) { return !(a == b); }
    constexpr static bool operator == (const uint32_t& a, const NameID& b) { return a == b.identifier; }
    constexpr static bool operator != (const uint32_t& a, const NameID& b) { return !(a == b); }

    namespace Hash
    {
        template<> struct THash<NameID> { size_t operator()(const NameID& k) const { return k.identifier; } };
    }
}

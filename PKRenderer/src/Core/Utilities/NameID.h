#pragma once
#include <cstdint>
#include <string>

namespace PK
{
    // Default implementation not provided as applications might have use different id layouts & memory strategies.
    // Additionally we dont want to expose data structure details here or make them static.
    // This would pollute includes & take allocation control away from the application.
    struct INameIDProvider
    {
        virtual uint32_t INameIDProvider_StringToID(const std::string& name) = 0;
        virtual const std::string& INameIDProvider_IDToString(const uint32_t& name) = 0;
    };

    struct NameID
    {
        uint32_t identifier = 0u;

        NameID() {}
        NameID(const char* name) : identifier(s_Provider->INameIDProvider_StringToID(name)) {}
        NameID(const std::string& name) : identifier(s_Provider->INameIDProvider_StringToID(name)) {}
        constexpr NameID(const NameID& name) : identifier(name.identifier) {}
        constexpr NameID(uint32_t identifier) : identifier(identifier) {}

        const std::string& to_string() const { return s_Provider->INameIDProvider_IDToString(identifier); }
        const char* c_str() const { return to_string().c_str(); }
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
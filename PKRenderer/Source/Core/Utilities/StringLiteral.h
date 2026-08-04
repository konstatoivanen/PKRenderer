#pragma once

namespace PK
{
    // Utilities should be independent from the rest of the project which is why these are declared here again.
    #if defined(__FUNCSIG__)
    #define PK_FUNC_SIG __FUNCSIG__
    #define PK_FUNC_SIG_LEN sizeof(__FUNCSIG__)
    #elif defined(__PRETTY_FUNCTION__) || defined(__clang__)
    #define PK_FUNC_SIG __PRETTY_FUNCTION__
    #define PK_FUNC_SIG_LEN sizeof(__PRETTY_FUNCTION__);
    #else
    #error "Unsupported compiler!"
    #endif

    consteval bool pk_char_is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_'); }
    consteval bool pk_char_is_numeric(char c) { return c >= '0' && c <= '9'; }
    consteval bool pk_char_is_alphanumeric(char c) { return pk_char_is_alpha(c) || pk_char_is_numeric(c); }

    template<size_t N>
    struct StringLiteral
    {
        constexpr static size_t length = N;
        char str[N + 1ull];
        constexpr const char* operator()() const& noexcept { return str; }
    };

    template<size_t N>
    struct StringLiteralArray
    {
        constexpr static const size_t size = N;
        const char* strings[N];
        constexpr const char* const* operator()() const& noexcept { return strings; }
    };
}
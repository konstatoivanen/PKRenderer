#pragma once
#include "Templates.h"

namespace PK
{
    // Utilities should be independent from the rest of the project which is why these are declared here again.
    #if defined(__FUNCSIG__)
    #define PK_FUNC_SIG __FUNCSIG__
    #define PK_FUNC_SIG_LEN sizeof(__FUNCSIG__)
    #define PK_FUNC_SIG_LEN_TRUNC (PK_FUNC_SIG_LEN - 17ull)
    #define PK_FUNC_SIG_LEN_TRUNC2 (PK_FUNC_SIG_LEN - 23ull)
    #elif defined(__PRETTY_FUNCTION__) || defined(__clang__)
    #define PK_FUNC_SIG __PRETTY_FUNCTION__
    #define PK_FUNC_SIG_LEN sizeof(__PRETTY_FUNCTION__)
    #define PK_FUNC_SIG_LEN_TRUNC (PK_FUNC_SIG_LEN - 2ull)
    #define PK_FUNC_SIG_LEN_TRUNC2 (PK_FUNC_SIG_LEN - 2ull)
    #else
    #error "Unsupported compiler!"
    #endif
    
    #define PK_SHORT_FUNCTION_NAME pk_short_function_name<StringLiteral<PK_FUNC_SIG_LEN>(PK_FUNC_SIG)>()

    template<size_t N>
    struct StringLiteral
    {
        constexpr static size_t length = N;
        char str[N + 1ull];

        constexpr StringLiteral(const char* src) noexcept { for (auto i = 0u; i < length; ++i) str[i] = src[i]; str[length] = '\0'; }
        constexpr const char* operator()() const& noexcept { return str; }
    };

    template<size_t N>
    struct StringLiteralArray
    {
        constexpr static const size_t size = N;
        const char* strings[N];
        constexpr const char* const* operator()() const& noexcept { return strings; }
    };

    struct StringLiteralView
    {
        const char* str;
        size_t length;
    };

    consteval bool pk_char_is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_'); }
    consteval bool pk_char_is_numeric(char c) { return c >= '0' && c <= '9'; }
    consteval bool pk_char_is_alphanumeric(char c) { return pk_char_is_alpha(c) || pk_char_is_numeric(c); }

    consteval bool pk_is_valid_func_sig(const char* name, size_t length) noexcept
    {
        if ((length >= 1 && (name[0] == '"' || name[0] == '\'')) ||
            (length >= 2 && name[0] == 'R' && (name[1] == '"' || name[1] == '\'')) ||
            (length >= 2 && name[0] == 'L' && (name[1] == '"' || name[1] == '\'')) ||
            (length >= 2 && name[0] == 'U' && (name[1] == '"' || name[1] == '\'')) ||
            (length >= 2 && name[0] == 'u' && (name[1] == '"' || name[1] == '\'')) ||
            (length >= 3 && name[0] == 'u' && name[1] == '8' && (name[2] == '"' || name[2] == '\'')) ||
            (length >= 1 && pk_char_is_numeric(name[0])))
        {
            return false;
        }

        return true;
    }

    template <StringLiteral Name> 
    inline constexpr auto pk_short_function_name = []() constexpr noexcept
    {
        constexpr auto view = []() consteval
        {
            auto name_begin = 0ull;
            auto name_begin_scoped = Name.length;

            auto i = 0ull;
            for (; i < Name.length && Name.str[i] != '\0' && Name.str[i] != '('; ++i)
            {
                auto is_name_char_0 = pk_char_is_alpha(Name.str[i + 0ull]);
                auto is_name_char_1 = pk_char_is_alpha(Name.str[i + 1ull]);
                if (!is_name_char_0 && is_name_char_1 && Name.str[i] == ':') name_begin_scoped = name_begin;
                if (!is_name_char_0 && is_name_char_1) name_begin = i + 1ull;
            }

            name_begin = name_begin < name_begin_scoped ? name_begin : name_begin_scoped;
            return StringLiteralView{ Name.str + name_begin, i - name_begin };
        }();

        return StringLiteral<view.length>(view.str);
    }();

    // Returns innermost type name. ie. T<N> -> N, if T is not a template this returns T
    template <typename T> 
    inline constexpr auto pk_inner_type_name = []<typename U>() constexpr noexcept
    {
        constexpr auto view = []<StringLiteral Name>() consteval
        {
            auto name = Name.str;
            auto length = Name.length;

            if (pk_is_valid_func_sig(name, length))
            {
                for (auto i = static_cast<int32_t>(length); i > 0; --i)
                {
                    if (pk_char_is_alpha(name[i - 1]))
                    {
                        length = i;
                        break;
                    }
                }

                auto s = 0;

                for (auto i = static_cast<int32_t>(length); i > 0; --i)
                {
                    if (!pk_char_is_alphanumeric(name[i - 1]))
                    {
                        s = i;
                        break;
                    }
                }

                name = name + s;
                length -= s;

                if (length > 0ull && pk_char_is_alpha(name[0]))
                {
                    return StringLiteralView{ name, length };
                }
            }

            return StringLiteralView{ nullptr, 0ull };
        }.template operator()<StringLiteral<PK_FUNC_SIG_LEN_TRUNC2>(PK_FUNC_SIG)>();

        return StringLiteral<view.length>(view.str);
    }.template operator()<T>();

    // Returns outermost typename. ie T<N> -> T
    template <typename T> 
    inline constexpr auto pk_outer_type_name =  []<typename U>() constexpr noexcept
    {
        constexpr auto view = []<StringLiteral Name>() consteval
        {
            auto name = Name.str;
            auto length = Name.length;

            if (!pk_is_valid_func_sig(name, length))
            {
                return StringLiteralView{ nullptr, 0ull };
            }

            for (auto i = static_cast<int32_t>(length), h = 0, s = 0; i > 0; --i)
            {
                if (name[i - 1] == ')') { ++h;++s; continue; }
                if (name[i - 1] == '(') { --h;++s; continue; }
                if (h == 0) { length -= s; break; }
                ++s;
            }

            auto s = 0;

            for (auto i = static_cast<int32_t>(length), h = 0; i > 0; --i)
            {
                if (name[i - 1] == '>') { ++h;++s; continue; }
                if (name[i - 1] == '<') { --h;++s; continue; }
                if (h == 0) { break; }
                ++s;
            }

            for (int32_t i = static_cast<int32_t>(length) - s; i > 0; --i)
            {
                if (!pk_char_is_alphanumeric(name[i - 1]))
                {
                    name = name + i;
                    length -= i;
                    break;
                }
            }

            length -= s;

            if (length > 0ull && pk_char_is_alpha(name[0]))
            {
                return StringLiteralView{ name, length };
            }

            return StringLiteralView{ nullptr, 0ull };
        }.template operator()<StringLiteral<PK_FUNC_SIG_LEN_TRUNC2>(PK_FUNC_SIG)>();

        return StringLiteral<view.length>(view.str);
    }.template operator()<T>();

    template <typename E, E V> 
    inline constexpr auto pk_enum_name = []<typename Enum, Enum Value>() constexpr noexcept
    {
        constexpr auto view = []<StringLiteral Name>() consteval
        {
            auto name = Name.str;
            auto length = Name.length;
            auto start = 0ull;

            for (auto i = static_cast<int32_t>(length); i > 0; --i)
            {
                if (!pk_char_is_alphanumeric(name[i - 1]))
                {
                    start = i;
                    break;
                }
            }

            length = name[start - 1ul] == ':' ? (length - start) : 0ull;
            return StringLiteralView{ name + start, length };
        }.template operator()<StringLiteral<PK_FUNC_SIG_LEN_TRUNC2>(PK_FUNC_SIG)>();

        return StringLiteral<view.length>(view.str);
    }.template operator()<E,V>();

    // Note do not use inside a dynamic library. 
    inline uint32_t pk_type_index_counter = 0u;

    template<typename T>
    inline const uint32_t pk_type_index = pk_type_index_counter++;

    template<typename T>
    constexpr uint32_t pk_base_type_index() { return pk_type_index<TRemoveCVRef_T<T>>; }
}
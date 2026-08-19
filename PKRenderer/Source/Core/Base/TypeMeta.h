#pragma once
#include "Core/Base/Types/UUID128.h"
#include "Templates.h"

namespace PK
{
    // Utilities should be independent from the rest of the project which is why these are declared here again.
    #if defined(__FUNCSIG__)
    #define PK_FUNC_SIG __FUNCSIG__
    #define PK_FUNC_SIG_LEN sizeof(__FUNCSIG__)
    #define PK_FUNC_SIG_LEN_TRUNC (PK_FUNC_SIG_LEN - 23ull)
    #elif defined(__PRETTY_FUNCTION__) || defined(__clang__)
    #define PK_FUNC_SIG __PRETTY_FUNCTION__
    #define PK_FUNC_SIG_LEN sizeof(__PRETTY_FUNCTION__)
    #define PK_FUNC_SIG_LEN_TRUNC (PK_FUNC_SIG_LEN - 2ull)
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
        constexpr auto view = []<StringLiteral Signature>() consteval
        {
            static_assert(pk_is_valid_func_sig(Signature.str, Signature.length), "Invalid function signature");

            auto name = Signature.str;
            auto length = Signature.length;

            for (auto i = length; i > 0ull; --i)
            {
                if (pk_char_is_alpha(name[i - 1ull]))
                {
                    length = i;
                    break;
                }
            }

            auto s = 0ull;

            for (auto i = length; i > 0ull; --i)
            {
                if (!pk_char_is_alphanumeric(name[i - 1ull]))
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

            return StringLiteralView{ nullptr, 0ull };
        }.template operator()<StringLiteral<PK_FUNC_SIG_LEN_TRUNC>(PK_FUNC_SIG)>();

        return StringLiteral<view.length>(view.str);
    }.template operator()<T>();

    // Returns outermost typename. ie T<N> -> T
    template <typename T> 
    inline constexpr auto pk_outer_type_name =  []<typename U>() constexpr noexcept
    {
        constexpr auto view = []<StringLiteral Signature>() consteval
        {
            static_assert(pk_is_valid_func_sig(Signature.str, Signature.length), "Invalid function signature");

            auto name = Signature.str;
            auto length = Signature.length;

            for (auto i = length, h = 0ull, s = 0ull; i > 0ull; --i)
            {
                if (name[i - 1ull] == ')') { ++h;++s; continue; }
                if (name[i - 1ull] == '(') { --h;++s; continue; }
                if (h == 0ull) { length -= s; break; }
                ++s;
            }

            auto s = 0ull;

            for (auto i = length, h = 0ull; i > 0ull; --i)
            {
                if (name[i - 1ull] == '>') { ++h;++s; continue; }
                if (name[i - 1ull] == '<') { --h;++s; continue; }
                if (h == 0ull) { break; }
                ++s;
            }

            for (auto i = length - s; i > 0ull; --i)
            {
                if (!pk_char_is_alphanumeric(name[i - 1ull]))
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
        }.template operator()<StringLiteral<PK_FUNC_SIG_LEN_TRUNC>(PK_FUNC_SIG)>();

        return StringLiteral<view.length>(view.str);
    }.template operator()<T>();

    // Returns full typename. ie T<N> -> T<N>
    template <typename T>
    inline constexpr auto pk_full_type_name = []<typename U>() constexpr noexcept
    {
        constexpr auto name = []<StringLiteral Signature>() consteval
        {
            static_assert(pk_is_valid_func_sig(Signature.str, Signature.length), "Invalid function signature");

            constexpr const char* keywords[3]{ "struct", "class", "enum" };
            constexpr const size_t keyword_lengths[3]{ 6u, 5u, 4u };

            StringLiteral<Signature.length> name(Signature.str);
            auto source = Signature.str;
            auto length = Signature.length;
            auto size = 0ull;

            // Find beginning
            for (auto i = length; i > 2ull; --i)
            {
                if ((source[i - 1ull] == '=') ||
                    (source[i - 1ull] == '<' && source[i - 2ull] == ')'))
                {
                    source += i;
                    length -= i;
                    break;
                }
            }

            // Filter keywords, namespaces and spaces.
            for (auto i = 0ull; i < length; ++i)
            {
                for (auto j = 0ull; j < 3ull; ++j)
                {
                    auto k = 0ull;
                    while (k < keyword_lengths[j] && source[i + k] == keywords[j][k]) { ++k; }

                    if (k == keyword_lengths[j])
                    {
                        i += k;
                        break;
                    }
                }

                if (pk_char_is_alphanumeric(source[i]))
                {
                    auto j = i + 1ull;
                    while (j < length && pk_char_is_alphanumeric(source[j])) { ++j; }

                    if (j < length && source[j] == ':')
                    {
                        i = j;
                    }
                }

                if (pk_char_is_alphanumeric(source[i]) || source[i] == '<' || source[i] == '>' || source[i] == ',')
                {
                    name.str[size++] = source[i];
                }
            }

            name.str[size] = '\0';
            return name;
        }.template operator() < StringLiteral<PK_FUNC_SIG_LEN_TRUNC>(PK_FUNC_SIG) > ();

        constexpr auto length = [](auto name) consteval
        {
            auto length = 0ull;
            while (length < name.length && name.str[length] != '\0') { ++length; }
            return length;
        }
        (name);

        return StringLiteral<length>(name.str);
    }.template operator()<T>();

    template <typename E, E V> 
    inline constexpr auto pk_enum_name = []<typename Enum, Enum Value>() constexpr noexcept
    {
        constexpr auto view = []<StringLiteral Signature>() consteval
        {
            static_assert(pk_is_valid_func_sig(Signature.str, Signature.length), "Invalid function signature");

            auto name = Signature.str;
            auto length = Signature.length;
            auto start = 0ull;

            for (auto i = length; i > 0ull; --i)
            {
                if (!pk_char_is_alphanumeric(name[i - 1ull]))
                {
                    start = i;
                    break;
                }
            }

            length = name[start - 1ull] == ':' ? (length - start) : 0ull;
            return StringLiteralView{ name + start, length };
        }.template operator()<StringLiteral<PK_FUNC_SIG_LEN_TRUNC>(PK_FUNC_SIG)>();

        return StringLiteral<view.length>(view.str);
    }.template operator()<E,V>();

    template <typename T>
    inline constexpr auto pk_type_uuid64 = []() constexpr noexcept
    {
        constexpr auto name = pk_full_type_name<T>;
        uint64_t value = 14695981039346656037ull;

        for (auto i = name.length; i > 0ull; --i)
        {
            value ^= static_cast<uint64_t>(name.str[i - 1ull]);
            value *= 1099511628211ull;
        }

        return value;
    }();

    template <typename T>
    inline constexpr auto pk_type_uuid128 = []() constexpr noexcept
    {
        constexpr auto name = pk_full_type_name<T>;

        UUID128 value;
        value.low = 0x62b821756295c58dull;
        value.high = 0x6c62272e07bb0142ull;

        // Reverse order as it is more likely to be unique in the end
        for (auto i = name.length; i > 0ull; --i)
        {
            value.low ^= static_cast<uint64_t>(name.str[i - 1ull]);
            auto low_low = (value.low & 0xFFFFFFFFull) * 0x13bull;
            auto low_high = (value.low >> 32ull) * 0x13bull;
            auto low_total = low_low + (low_high << 32ull);
            auto carry = ((low_low >> 32ull) + low_high) >> 32ull;
            value.high = (value.high * 0x13bull) + (value.low * 0x1000000ull) + carry;
            value.low = low_total;
        }

        return value;
    }();

    // Note do not use inside a dynamic library. 
    inline uint32_t pk_type_index_counter = 0u;

    template<typename T>
    inline const uint32_t pk_type_index = pk_type_index_counter++;

    template<typename T>
    constexpr uint32_t pk_base_type_index() { return pk_type_index<TRemoveCVRef_T<T>>; }

    #if PK_DEBUG
    template <typename... Types>
    struct ValidateTypeUUIDs 
    {
        consteval static bool ValidateHashes64()
        {
            constexpr uint64_t hashes[] = { pk_type_uuid64<Types>... };
            constexpr size_t N = sizeof...(Types);

            for (auto i = 0ull; i < N; ++i) 
            for (auto j = i + 1ull; j < N; ++j)
            {
                if (hashes[i] == hashes[j]) 
                {
                    return false; 
                }
            }

            return true;
        }

        consteval static bool ValidateHashes128()
        {
            constexpr UUID128 hashes[] = { pk_type_uuid128<Types>... };
            constexpr size_t N = sizeof...(Types);

            for (auto i = 0ull; i < N; ++i)
            for (auto j = i + 1ull; j < N; ++j)
            {
                if (hashes[i] == hashes[j])
                {
                    return false;
                }
            }

            return true;
        }

        static_assert(ValidateHashes64(), "UUID64 hash collision!");
        static_assert(ValidateHashes128(), "UUID128 hash collision!");
    };
    #endif
}

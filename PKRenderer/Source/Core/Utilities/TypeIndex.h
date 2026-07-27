#pragma once
#include "Templates.h"
#include "BufferView.h"

namespace PK
{
    // Note do not use inside a dynamic library. 
    inline uint32_t pk_type_index_counter = 0u;
    
    template<typename T> 
    inline const uint32_t pk_type_index = pk_type_index_counter++;

    template<typename T>
    constexpr uint32_t pk_base_type_index() { return pk_type_index<TRemoveCVRef_T<T>>; }

    consteval bool pk_char_is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_'); }
    consteval bool pk_char_is_numeric(char c) { return c >= '0' && c <= '9'; }
    consteval bool pk_char_is_alphanumeric(char c) { return pk_char_is_alpha(c) || pk_char_is_numeric(c); }

    consteval bool pk_type_name_validate(const char* name, size_t length) noexcept
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

    // Source: https://github.com/Neargye/nameof
    // Returns outermost typename. ie T<N> -> T
    consteval ConstBufferView<char> pk_outer_type_name_view(const char* name, size_t length) noexcept
    {
        if (!pk_type_name_validate(name, length))
        {
            return { nullptr, 0ull };
        }

        for (int32_t i = static_cast<int32_t>(length), h = 0, s = 0; i > 0; --i)
        {
            if (name[i - 1] == ')')
            {
                ++h;
                ++s;
                continue;
            }

            if (name[i - 1] == '(')
            {
                --h;
                ++s;
                continue;
            }

            if (h == 0)
            {
                length -= s;
                break;
            }
            ++s;
        }

        int32_t s = 0;

        for (int32_t i = static_cast<int32_t>(length), h = 0; i > 0; --i)
        {
            if (name[i - 1] == '>')
            {
                ++h;
                ++s;
                continue;
            }

            if (name[i - 1] == '<')
            {
                --h;
                ++s;
                continue;
            }

            if (h == 0)
            {
                break;
            }

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
            return { name, length };
        }

        return { nullptr, 0ull };
    }

    // Returns innermost type name. ie. T<N> -> N, if T is not a template it will return T.
    consteval ConstBufferView<char> pk_inner_type_name_view(const char* name, size_t length) noexcept
    {
        if (!pk_type_name_validate(name, length))
        {
            return { name, length };
        }

        for (auto i = static_cast<int32_t>(length); i > 0; --i)
        {
            if (pk_char_is_alpha(name[i - 1]))
            {
                length = i;
                break;
            }
        }
        
        int32_t s = 0;

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
            return { name, length };
        }

        return { nullptr, 0ull };
    }

    template<typename T>
    consteval ConstBufferView<char> pk_base_type_name() noexcept
    {
    #if defined(__clang__)
        return pk_outer_type_name_view(__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__) - 2);
    #elif defined(_MSC_VER)
        return pk_outer_type_name_view(__FUNCSIG__, sizeof(__FUNCSIG__) - 17);
    #else
        #error "Unsupported compiler!"
    #endif
    }

    template<typename T>
    consteval ConstBufferView<char> pk_inner_type_name() noexcept
    {
    #if defined(__clang__)
        return pk_inner_type_name_view(__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__) - 2);
    #elif defined(_MSC_VER)
        return pk_inner_type_name_view(__FUNCSIG__, sizeof(__FUNCSIG__) - 17);
    #else
        #error "Unsupported compiler!"
        #endif
    }
}

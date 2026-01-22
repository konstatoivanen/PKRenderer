#pragma once
#include <cstdint>
#include "BufferView.h"

namespace PK
{
    // Note do not use inside a dynamic library. 
    inline uint32_t pk_type_index_counter = 0u;
    
    template<typename T> 
    inline const uint32_t pk_type_index = pk_type_index_counter++;

    template<typename T>
    constexpr uint32_t pk_base_type_index()
    {
        using TBase = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
        return pk_type_index<TBase>;
    }

    // Source: https://github.com/Neargye/nameof
    constexpr ConstBufferView<char> pk_base_type_name(const char* name, size_t length) noexcept
    {
        if ((length >= 1 && (name[0] == '"' || name[0] == '\'')) ||
            (length >= 2 && name[0] == 'R' && (name[1] == '"' || name[1] == '\'')) ||
            (length >= 2 && name[0] == 'L' && (name[1] == '"' || name[1] == '\'')) ||
            (length >= 2 && name[0] == 'U' && (name[1] == '"' || name[1] == '\'')) ||
            (length >= 2 && name[0] == 'u' && (name[1] == '"' || name[1] == '\'')) ||
            (length >= 3 && name[0] == 'u' && name[1] == '8' && (name[2] == '"' || name[2] == '\'')) ||
            (length >= 1 && (name[0] >= '0' && name[0] <= '9')))
        {
            return { nullptr, 0ull };
        }

        for (int32_t i = length, h = 0, s = 0; i > 0; --i)
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

        for (int32_t i = length, h = 0; i > 0; --i)
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

        for (int32_t i = length - s; i > 0; --i)
        {
            if (!((name[i - 1] >= '0' && name[i - 1] <= '9') || (name[i - 1] >= 'a' && name[i - 1] <= 'z') || (name[i - 1] >= 'A' && name[i - 1] <= 'Z') || (name[i - 1] == '_')))
            {
                name = name + i;
                length -= i;
                break;
            }
        }

        length -= s;

        if (length > 0 && ((name[0] >= 'a' && name[0] <= 'z') || (name[0] >= 'A' && name[0] <= 'Z') || (name[0] == '_')))
        {
            return { name, length };
        }

        return { nullptr, 0ull };
    }

    template<typename T>
    constexpr ConstBufferView<char> pk_base_type_name() noexcept
    {
    #if defined(__clang__)
        return pk_base_type_name(__PRETTY_FUNCTION__, sizeof(__PRETTY_FUNCTION__) - 2);
    #elif defined(_MSC_VER)
        return pk_base_type_name(__FUNCSIG__, sizeof(__FUNCSIG__) - 17);
    #else
        #error "Unsupported compiler!"
    #endif
    }
}

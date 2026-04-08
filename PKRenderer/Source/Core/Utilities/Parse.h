#pragma once
#include "FixedString.h"

namespace PK::Parse
{
    FixedString32 BytesToString(size_t bytes);
    
    FixedString64 GetFilePathStem(const char* str);

    constexpr size_t GetFilePathDirectoryLength(const char* str)
    {
        auto length = 0u;

        for (auto i = 0u; i < 256ull && str[i] != '\0'; ++i)
        {
            if (str[i] == '\\' || str[i] == '/')
            {
                length = i + 1u;
            }
        }

        return length;
    }

    constexpr void GetShortFunctionName(const char* str, const char** outData, size_t* outLength) noexcept
    {
        auto name_begin = 0ull;
        auto name_begin_scoped = 256ull;

        auto i = 0ull;
        for (; i < 256ull && str[i] != '\0' && str[i] != '('; ++i)
        {
            auto is_name_char_0 = (str[i + 0ull] >= 'a' && str[i + 0ull] <= 'z') || (str[i + 0ull] >= 'A' && str[i + 0ull] <= 'Z') || (str[i + 0ull] == '_');
            auto is_name_char_1 = (str[i + 1ull] >= 'a' && str[i + 1ull] <= 'z') || (str[i + 1ull] >= 'A' && str[i + 1ull] <= 'Z') || (str[i + 1ull] == '_');
            if (!is_name_char_0 && is_name_char_1 && str[i] == ':') name_begin_scoped = name_begin;
            if (!is_name_char_0 && is_name_char_1) name_begin = i + 1ull;
        }

        name_begin = name_begin < name_begin_scoped ? name_begin : name_begin_scoped;
        *outData = str + name_begin;
        *outLength = i - name_begin;
    }

    constexpr const char* GetShortFunctionNameData(const char* str) noexcept
    {
        const char* data = str;
        size_t length = 0ull;
        GetShortFunctionName(str, &data, &length);
        return data;
    }

    constexpr size_t GetShortFunctionNameLength(const char* str) noexcept
    {
        const char* begin = str;
        size_t length = 0ull;
        GetShortFunctionName(str, &begin, &length);
        return length;
    }
}

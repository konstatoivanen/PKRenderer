#pragma once
#include <string>

// Forward declared to avoid dependency bloat.
namespace PK
{
    template<size_t capacity>
    struct FixedString;
    typedef FixedString<32> FixedString32;
    typedef FixedString<64> FixedString64;
}

namespace PK::Parse
{
    std::string FormatToString(const char* format, ...);

    template<typename T>
    T FromString(const char* str);

    template<typename T>
    FixedString32 ToString(const T& value);

    template<typename T>
    void StringsToArray(const char* const* strs, T* outArray, const uint32_t count)
    {
        for (auto i = 0u; i < count; ++i)
        {
            outArray[i] = FromString<T>(strs[i]);
        }
    }

    template<typename T>
    void StringToArray(const char* str, T* outArray, const uint32_t count)
    {
        auto length = strlen(str);

        if (length == 0)
        {
            return;
        }

        auto scope0 = strchr(str, '[');
        auto scope1 = strchr(str, ']');

        if (scope0 == nullptr || scope1 == nullptr || scope1 <= scope0)
        {
            return;
        }

        auto strBegin = scope0 + 1u;
        auto strCount = 0ull;
        auto index = 0u;

        for (auto cur = scope0 + 1u; cur <= scope1 && index < count; ++cur, ++strCount)
        {
            if (*cur == ',' || cur == scope1)
            {
                outArray[index++] = FromString<T>(std::string(strBegin, strCount).c_str());
                strBegin = cur + 1u;
                strCount = 0u;
            }
        }
    }

    template<typename T>
    std::string ArrayToString(const T* array, const uint32_t count)
    {
        std::string result;

        for (auto i = 0u; i < count; ++i)
        {
            result.append(ToString(array[i]));

            if (i != (count - 1))
            {
                result.append(" ");
            }
        }

        return result;
    }

    std::wstring ToWideString(const char* str, size_t length);
    std::string FromWideString(const wchar_t* str, size_t length);

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

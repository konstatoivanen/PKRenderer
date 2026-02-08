#include "PrecompiledHeader.h"
#include "FixedString.h"
#include "Parse.h"

namespace PK::Parse
{
    std::string FormatToString(const char* format, ...)
    {
        va_list v0, v1;

        va_start(v0, format);
        auto size = _vsnprintf(nullptr, 0, format, v0);
        va_end(v0);

        if (size < 0)
        {
            return std::string();
        }

        std::string value = std::string(size, '0');
        
        va_start(v1, format);
        _vsnprintf(value.data(), (size_t)size + 1u, format, v1);
        va_end(v1);

        return value;
    }

    template<> uint8_t FromString(const char* str) { return (uint8_t)atoi(str); }
    template<> int8_t FromString(const char* str) { return (int8_t)atoi(str); }
    template<> uint16_t FromString(const char* str) { return (uint16_t)atoi(str); }
    template<> int16_t FromString(const char* str) { return (int16_t)atoi(str); }
    template<> uint32_t FromString(const char* str) { return (uint32_t)atoi(str); }
    template<> int32_t FromString(const char* str) { return (int32_t)atoi(str); }
    template<> uint64_t FromString(const char* str) { return (uint64_t)atoll(str); }
    template<> int64_t FromString(const char* str) { return (int64_t)atoll(str); }
    template<> float FromString(const char* str) { return (float)atof(str); }
    template<> bool FromString(const char* str) { return strcmp(str, "True") == 0 ? true : (strcmp(str, "False") == 0 ? false : (bool)atoi(str)); }

    template<> FixedString32 ToString(const uint8_t& value) { return FixedString32("%u", value); }
    template<> FixedString32 ToString(const int8_t& value) { return FixedString32("%i", value); }
    template<> FixedString32 ToString(const uint16_t& value) { return FixedString32("%u", value); }
    template<> FixedString32 ToString(const int16_t& value) { return FixedString32("%i", value); }
    template<> FixedString32 ToString(const uint32_t& value) { return FixedString32("%u", value); }
    template<> FixedString32 ToString(const int32_t& value) { return FixedString32("%i", value); }
    template<> FixedString32 ToString(const uint64_t& value) { return FixedString32("%llu", value); }
    template<> FixedString32 ToString(const int64_t& value) { return FixedString32("%lli", value); }
    template<> FixedString32 ToString(const float& value) { return FixedString32("%fg", value); }
    template<> FixedString32 ToString(const bool& value) { return FixedString32("%u", (uint8_t)value); }

    std::wstring ToWideString(const char* str, size_t length)
    {
        std::wstring wide(length, L'#');
        mbstowcs(wide.data(), str, length);
        return wide;
    }

    std::string FromWideString(const wchar_t* str, size_t length)
    {
        std::string narrow(length, '#');
        wcstombs(narrow.data(), str, length);
        return narrow;
    }

    FixedString64 GetFilePathStem(const char* str)
    {
        auto last_slash = 0ull;
        auto last_dot = 0ull;
        auto i = 0ull;

        for (; i < 256ull && str[i] != '\0'; ++i)
        {
            if (str[i] == '/' || str[i] == '\\')
            {
                last_slash = i + 1ull;
            }

            if (str[i] == '.')
            {
                last_dot = i;
            }
        }

        return FixedString64(last_dot - last_slash, str + last_slash);
    }
}
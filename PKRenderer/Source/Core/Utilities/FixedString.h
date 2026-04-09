#pragma once
#include <wchar.h>
#include <string.h>
#include <stdarg.h>
#include "Hash.h"
#include "Memory.h"

namespace PK
{
    template<typename TChar, size_t capacity>
    struct IFixedString
    {
        constexpr const static size_t max_length = capacity - 1ull;

        constexpr IFixedString() : m_length(0ull)
        {
            m_string[m_length] = '\0';
            m_string[max_length] = '\0';
        }

        template<typename TOther = TChar>
        IFixedString(size_t length, const TOther* str) : IFixedString()
        {
            if (str && str[0])
            {
                m_length = max_length > length ? length : max_length;

                if constexpr (__is_same(TOther, TChar))
                {
                    String::Copy(m_string, str, m_length);
                }
                else if constexpr (__is_same(TOther, wchar_t))
                {
                    String::ToNarrow(m_string, str, m_length);
                }
                else if constexpr (__is_same(TOther, char))
                {
                    String::ToWide(m_string, str, m_length);
                }

                m_string[m_length] = '\0';
            }
        }

        IFixedString(const TChar* format, ...) : IFixedString()
        {
            if (format && format[0])
            {
                va_list v0;
                va_start(v0, format);
                m_length = String::Format(m_string, capacity, format, v0);
                m_length = max_length > m_length ? m_length : max_length;
                va_end(v0);
            }
        }

        IFixedString(std::initializer_list<const TChar*> strings) : IFixedString()
        {
            for (auto& str : strings)
            {
                if (m_length < max_length && str&& str[0])
                {
                    String::Copy(m_string + m_length, str, max_length - m_length);
                    m_length += String::Length(str);
                }
            }

            m_length = max_length > m_length ? m_length : max_length;
        }

        constexpr TChar& operator [](size_t i) { return m_string[i]; }
        constexpr const TChar& operator [](size_t i) const { return m_string[i]; }

        constexpr operator TChar* () { return c_str(); }
        constexpr operator const TChar* () const { return c_str(); }

        bool operator == (const TChar* str) { return String::Cmp(str, m_string) == 0; }
        bool operator != (const TChar* str) { return String::Cmp(str, m_string) != 0; }

        constexpr TChar* begin() { return m_string; }
        constexpr TChar* end() { return m_string + m_length; }
        constexpr TChar const* begin() const { return m_string; }
        constexpr TChar const* end() const { return m_string + m_length; }
        constexpr TChar* c_str() { return m_string; }
        constexpr const TChar* c_str() const { return m_string; }

        constexpr size_t Length() const { return m_length; }
        constexpr TChar Front() const { return m_string[0]; }
        constexpr TChar Back() const { return m_string[m_length > 0ull ? m_length - 1ull : 0u]; }
        constexpr bool IsFull() const { return m_length == max_length; }

        int64_t Find(size_t offset, TChar c) const { return reinterpret_cast<int64_t>(String::Chr(m_string + offset, c) - m_string); }
        IFixedString Slice(size_t offset, size_t count = max_length) const { return IFixedString(offset + count > m_length ? m_length - offset : count, m_string + offset); }

        void AppendFormat(const TChar* format, ...)
        {
            if (format && format[0])
            {
                va_list v0;
                va_start(v0, format);
                m_length += String::Format(m_string + m_length, capacity - m_length, format, v0);
                m_length = max_length > m_length ? m_length : max_length;
                va_end(v0);
            }
        }

        void Append(const TChar* str)
        {
            if (str && str[0])
            {
                String::Copy(m_string + m_length, str, max_length - m_length);
                m_length += String::Length(str);
                m_length = max_length > m_length ? m_length : max_length;
            }
        }

        constexpr void Append(TChar c)
        {
            if (c && m_length < max_length)
            {
                m_string[m_length] = c;
                m_string[++m_length] = '\0';
            }
        }

        constexpr TChar Pop()
        {
            auto c = m_string[m_length];
            m_string[m_length ? --m_length : 0ull] = '\0';
            return c;
        }

        constexpr void Clear()
        {
            m_string[0] = '\0';
            m_length = 0;
        }

    private:
        TChar m_string[capacity];
        size_t m_length;
    };

    template<size_t capacity> using FixedString = IFixedString<char, capacity>;
    template<size_t capacity> using FixedWString = IFixedString<wchar_t, capacity>;

    typedef FixedString<16> FixedString16;
    typedef FixedString<32> FixedString32;
    typedef FixedString<64> FixedString64;
    typedef FixedString<128> FixedString128;
    typedef FixedString<256> FixedString256;
    typedef FixedString<512> FixedString512;
    typedef FixedString<1024> FixedString1024;

    typedef FixedWString<16> FixedWString16;
    typedef FixedWString<32> FixedWString32;
    typedef FixedWString<64> FixedWString64;
    typedef FixedWString<128> FixedWString128;
    typedef FixedWString<256> FixedWString256;
    typedef FixedWString<512> FixedWString512;
    typedef FixedWString<1024> FixedWString1024;

    namespace Hash
    {
        template<size_t capacity>
        struct THash<FixedString<capacity>>
        {
            size_t operator()(const FixedString<capacity>& str) const noexcept
            {
                return FNV1AHash(str.c_str(), str.Length());
            }
        };

        template<size_t capacity>
        struct THash<FixedWString<capacity>>
        {
            size_t operator()(const FixedWString<capacity>& str) const noexcept
            {
                return FNV1AHash(str.c_str(), str.Length() * sizeof(wchar_t));
            }
        };
    }

    namespace String
    {
        inline wchar_t* Copy(wchar_t* dst, const wchar_t* src, size_t size) { return wcsncpy(dst, src, size); }
        inline char* Copy(char* dst, const char* src, size_t size) { return strncpy(dst, src, size); }
        inline const wchar_t* Chr(const wchar_t* str0, wchar_t c) { return wcschr(str0, c); }
        inline const char* Chr(const char* str0, char c) { return strchr(str0, c); }
        inline size_t Format(wchar_t* dst, size_t size, const wchar_t* format, va_list args) { return static_cast<size_t>(vswprintf(dst, size, format, args)); }
        inline size_t Format(char* dst, size_t size, const char* format, va_list args) { return static_cast<size_t>(vsnprintf(dst, size, format, args)); }
        inline size_t Length(const wchar_t* str) { return wcslen(str); }
        inline size_t Length(const char* str) { return strlen(str); }
        inline int32_t Cmp(const wchar_t* str0, const wchar_t* str1) { return wcscmp(str0, str1); }
        inline int32_t Cmp(const char* str0, const char* str1) { return strcmp(str0, str1); }
        inline size_t ToWide(wchar_t* dst, const char* src, size_t size) { return mbstowcs(dst, src, size); }
        inline size_t ToNarrow(char* dst, const wchar_t* src, size_t size) { return wcstombs(dst, src, size); }

        template<typename T> inline T To(const char* str) { return T(); }
        template<> inline uint8_t To(const char* str) { return (uint8_t)atoi(str); }
        template<> inline int8_t To(const char* str) { return (int8_t)atoi(str); }
        template<> inline uint16_t To(const char* str) { return (uint16_t)atoi(str); }
        template<> inline int16_t To(const char* str) { return (int16_t)atoi(str); }
        template<> inline uint32_t To(const char* str) { return (uint32_t)atoi(str); }
        template<> inline int32_t To(const char* str) { return (int32_t)atoi(str); }
        template<> inline uint64_t To(const char* str) { return (uint64_t)atoll(str); }
        template<> inline int64_t To(const char* str) { return (int64_t)atoll(str); }
        template<> inline float To(const char* str) { return (float)atof(str); }
        template<> inline bool To(const char* str) { return strcmp(str, "True") == 0 ? true : (strcmp(str, "False") == 0 ? false : (bool)atoi(str)); }

        template<typename T> inline FixedString32 From(const T& value) { return FixedString32("unsupported"); }
        template<> inline FixedString32 From(const uint8_t& value) { return FixedString32("%u", value); }
        template<> inline FixedString32 From(const int8_t& value) { return FixedString32("%i", value); }
        template<> inline FixedString32 From(const uint16_t& value) { return FixedString32("%u", value); }
        template<> inline FixedString32 From(const int16_t& value) { return FixedString32("%i", value); }
        template<> inline FixedString32 From(const uint32_t& value) { return FixedString32("%u", value); }
        template<> inline FixedString32 From(const int32_t& value) { return FixedString32("%i", value); }
        template<> inline FixedString32 From(const uint64_t& value) { return FixedString32("%llu", value); }
        template<> inline FixedString32 From(const int64_t& value) { return FixedString32("%lli", value); }
        template<> inline FixedString32 From(const float& value) { return FixedString32("%fg", value); }
        template<> inline FixedString32 From(const bool& value) { return FixedString32("%u", (uint8_t)value); }

        template<typename T>
        void ToArray(const char* const* strs, T* outArray, const uint32_t count)
        {
            for (auto i = 0u; i < count; ++i)
            {
                outArray[i] = String::To<T>(strs[i]);
            }
        }

        template<typename T>
        void ToArray(const char* str, T* outArray, const uint32_t count)
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
                    outArray[index++] = String::To<T>(FixedString32(strCount, strBegin).c_str());
                    strBegin = cur + 1u;
                    strCount = 0u;
                }
            }
        }

        template<typename T, size_t size>
        FixedString<size> FromArray(const T* array, const uint32_t count)
        {
            FixedString<size> result;

            for (auto i = 0u; i < count; ++i)
            {
                result.Append(String::From(array[i]).c_str());

                if (i != (count - 1))
                {
                    result.Append(' ');
                }
            }

            return result;
        }

        // Some utilities that are not really relevant here but I'd rather not add another header
        template<size_t capacity>
        FixedString<capacity> FromBytes(size_t bytes)
        {
            if (bytes == 0)
            {
                return FixedString<capacity>("0B");
            }

            auto mag = (int)(log(bytes) / log(1024));
            mag = mag < 2 ? 2 : mag;

            auto adjustedSize = (double)bytes / (1L << (mag * 10));
            auto factor = pow(10, 4);

            if ((round(adjustedSize * factor) / factor) >= 1000)
            {
                mag += 1;
                adjustedSize /= 1024;
            }

            FixedString<capacity> str("%1.4g", adjustedSize);

            switch (mag)
            {
                case 0: str.Append('B'); break;
                case 1: str.Append('K'); str.Append('B'); break;
                case 2: str.Append('M'); str.Append('B'); break;
                default: str.Append('G'); str.Append('B'); break;
            }

            return str;
        }

        template<size_t capacity>
        inline constexpr FixedString<capacity> ToFilePathStem(const char* str)
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

            FixedString<capacity> stem;

            for (i = last_slash; i < 256ull && i < last_dot; ++i)
            {
                stem.Append(str[i]);
            }

            return stem;
        }

        inline constexpr size_t ToFilePathDirectoryLength(const char* str)
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

        inline constexpr void ToFunctionName(const char* str, const char** outData, size_t* outLength) noexcept
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

        inline constexpr const char* ToFunctionNameBase(const char* str) noexcept
        {
            const char* data = str;
            size_t length = 0ull;
            ToFunctionName(str, &data, &length);
            return data;
        }

        inline constexpr size_t ToFunctionNameLength(const char* str) noexcept
        {
            const char* begin = str;
            size_t length = 0ull;
            ToFunctionName(str, &begin, &length);
            return length;
        }

        template<typename TCharOut, typename TCharIn, size_t capacity>
        IFixedString<TCharOut, capacity> ToParentPath(const TCharIn* str)
        {
            auto parent_slash = 0ull;
            auto last_slash = 0ull;
            auto i = 0ull;

            for (; i < 256ull && str[i] != '\0'; ++i)
            {
                if (str[i] == '/' || str[i] == '\\')
                {
                    parent_slash = last_slash;
                    last_slash = i + 1ull;
                }
            }

            if (str[last_slash] == '\0')
            {
                last_slash = parent_slash;
            }

            IFixedString<TCharOut, capacity> parent;

            for (i = 0; i < 256ull && i < last_slash; ++i)
            {
                parent.Append(str[i]);
            }

            return parent;
        }

        template<typename TCHar>
        inline bool IsPathAbsolute(const TCHar* path)
        {
            return path && path[0] && path[1] && (((isdigit(path[0]) || isalpha(path[0])) && path[1] == ':') || (path[0] == '\\') || (path[0] == '/'));
        }
    }
}

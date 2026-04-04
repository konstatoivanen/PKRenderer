#pragma once
#include "Hash.h"
#include "Memory.h"

namespace PK
{
    template<size_t capacity>
    struct FixedString
    {
        constexpr const static size_t end = capacity - 1ull;

        FixedString() 
        {
            m_string[0] = '\0';
            m_string[end] = '\0';
            m_length = 0u;
        }

        FixedString(size_t length, const char* str) : FixedString()
        {
            if (str && str[0])
            {
                m_length = end > length ? length : end;
                strncpy(m_string, str, m_length);
                m_string[m_length] = '\0';
            }
        }

        FixedString(const char* format, ...) : FixedString()
        {
            if (format && format[0])
            {
                va_list v0;
                va_start(v0, format);
                m_length = _vsnprintf(m_string, capacity, format, v0);
                m_length = end > m_length ? m_length : end;
                va_end(v0);
            }
        }

        FixedString(std::initializer_list<const char*> strings) : FixedString()
        {
            for (auto& str : strings)
            {
                if (m_length < end && str && str[0])
                {
                    strncpy(m_string + m_length, str, end - m_length);
                    m_length += strlen(str);
                }
            }

            m_length = end > m_length ? m_length : end;
        }

        constexpr size_t Length() const { return m_length; }
        constexpr char Back() const { return m_length == 0 ? '\0' : m_string[m_length - 1u]; }
        constexpr bool IsFull() const { return m_length == capacity - 1ull; }
        constexpr const char* c_str() const { return m_string; }
        char* c_str() { return m_string; }

        char& operator [](size_t i) { return m_string[i]; }
        const char& operator [](size_t i) const { return m_string[i]; }

        operator char* () { return c_str(); }
        operator const char* () const { return c_str(); }

        bool operator == (const char* str) { return strcmp(str, m_string) == 0; }
        bool operator != (const char* str) { return strcmp(str, m_string) != 0; }

        int64_t FindPos(size_t offset, char c) const { return reinterpret_cast<int64_t>(strchr(m_string + offset, c) - m_string); }
        FixedString SubString(size_t offset, size_t count) const { return FixedString(count, m_string + offset); }

        void Append(const char* str)
        {
            if (str && str[0])
            {
                strncpy(m_string + m_length, str, end - m_length);
                m_length += strlen(str);
                m_length = end > m_length ? m_length : end;
            }
        }

        void AppendFormat(const char* format, ...)
        {
            if (format && format[0])
            {
                va_list v0;
                va_start(v0, format);
                m_length += _vsnprintf(m_string + m_length, capacity - m_length, format, v0);
                m_length = end > m_length ? m_length : end;
                va_end(v0);
            }
        }

        void Append(char c) 
        {
            if (c && m_length < end)
            {
                m_string[m_length] = c;
                m_string[++m_length] = 0;
            }
        }

        char Pop()
        {
            auto c = m_string[m_length];
            m_string[m_length ? --m_length : 0ull] = 0;
            return c;
        }

        void Clear()
        {
            m_string[0] = '\0';
            m_length = 0;
        }

    private:
        char m_string[capacity];
        size_t m_length;
    };

    typedef FixedString<16> FixedString16;
    typedef FixedString<32> FixedString32;
    typedef FixedString<64> FixedString64;
    typedef FixedString<128> FixedString128;
    typedef FixedString<256> FixedString256;
    typedef FixedString<512> FixedString512;
    typedef FixedString<1024> FixedString1024;

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
    }
}

#pragma once
#include "NoCopy.h"
#include "Hash.h"
#include <exception>

namespace PK
{
    template<size_t capacity>
    struct FixedString
    {
        FixedString() {}

        FixedString(size_t length, const char* str)
        {
            if (str == nullptr)
            {
                return;
            }

            m_length = length;

            if (m_length + 1u >= capacity)
            {
                throw std::exception("FixedString capacity exceeded!");
            }

            strncpy(m_string, str, m_length);
            m_string[m_length] = 0;
        }

        FixedString(const char* format, ...)
        {
            if (format == nullptr)
            {
                return;
            }

            va_list v0;
            va_start(v0, format);
            m_length = _vsnprintf(m_string, capacity, format, v0);
            m_string[capacity - 1] = 0;
            va_end(v0);

            if (m_length + 1u >= capacity)
            { 
                throw std::exception("FixedString capacity exceeded!");
            }
        }

        FixedString(std::initializer_list<const char*> strings)
        {
            for (auto& str : strings)
            {
                auto offset = m_length;
                m_length += strlen(str);

                if (m_length + 1u >= capacity)
                {
                    throw std::exception("FixedString capacity exceeded!");
                }

                strcpy(m_string + offset, str);
            }
        }

        void Append(const char* str)
        {
            auto offset = m_length;
            m_length += strlen(str);

            if (m_length + 1u >= capacity)
            {
                throw std::exception("FixedString capacity exceeded!");
            }

            strcpy(m_string + offset, str);
        }

        char& operator [](size_t i) { return m_string[i]; }
        const char& operator [](size_t i) const { return m_string[i]; }
        constexpr size_t Length() const { return m_length; }
        constexpr const char* c_str() const { return m_string; }
        char* c_str() { return m_string; }

        operator char* () { return c_str(); }
        operator const char* () const { return c_str(); }

        bool operator == (const char* str) { return strcmp(str, m_string) == 0; }
        bool operator != (const char* str) { return strcmp(str, m_string) != 0; }

    private:
        char m_string[capacity];
        size_t m_length = 0ull;
    };

    typedef FixedString<32> FixedString32;
    typedef FixedString<64> FixedString64;
    typedef FixedString<128> FixedString128;
    typedef FixedString<256> FixedString256;
    typedef FixedString<512> FixedString512;
}

namespace std 
{
    template <size_t capacity>
    struct hash<PK::FixedString<capacity>> 
    {
        size_t operator()(const PK::FixedString<capacity>& str) const noexcept
        { 
            return PK::Hash::FNV1AHash(str.c_str(), str.Length());
        }
    };
}

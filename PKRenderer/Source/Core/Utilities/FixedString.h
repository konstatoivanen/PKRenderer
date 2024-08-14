#pragma once
#include "NoCopy.h"
#include "Parsing.h"
#include <exception>

namespace PK
{
    template<size_t capacity>
    struct FixedString
    {
        FixedString() { m_length = 0ull; }

        FixedString(const char* str)
        {
            m_length = strlen(str);

            if (m_length + 1u >= capacity)
            {
                throw std::exception("FixedString capacity exceeded!");
            }

            std::copy(str, str + m_length + 1u, m_string);
        }

        FixedString(const char* format...)
        {
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
                auto length = strlen(str);
                m_length += strlen(str);

                if (m_length + 1u >= capacity)
                {
                    throw std::exception("FixedString capacity exceeded!");
                }

                std::copy(str, str + length + 1u, m_string + offset);
            }
        }

        void Append(const char* str)
        {
            auto offset = m_length;
            auto length = strlen(str);
            m_length += strlen(str);

            if (m_length + 1u >= capacity)
            {
                throw std::exception("FixedString capacity exceeded!");
            }

            std::copy(str, str + length + 1u, m_string + offset);
        }

        char& operator [](size_t i) { return m_string[i]; }
        const char& operator [](size_t i) const { return m_string[i]; }
        constexpr size_t Length() const { return m_length; }
        constexpr const char* c_str() const { return m_string; }
        char* c_str() { return m_string; }

    private:
        char m_string[capacity];
        size_t m_length = 0ull;
    };

    typedef FixedString<64> FixedString64;
    typedef FixedString<128> FixedString128;
    typedef FixedString<256> FixedString256;
    typedef FixedString<512> FixedString512;
}
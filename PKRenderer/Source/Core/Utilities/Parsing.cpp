#include "PrecompiledHeader.h"
#include "Parsing.h"

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

    template<> std::string ToString(const uint8_t& value) { char str[32]; sprintf(str, "%u", value); return std::string(str); }
    template<> std::string ToString(const int8_t& value) { char str[32]; sprintf(str, "%i", value); return std::string(str); }
    template<> std::string ToString(const uint16_t& value) { char str[32]; sprintf(str, "%u", value); return std::string(str); }
    template<> std::string ToString(const int16_t& value) { char str[32]; sprintf(str, "%i", value); return std::string(str); }
    template<> std::string ToString(const uint32_t& value) { char str[32]; sprintf(str, "%u", value); return std::string(str); }
    template<> std::string ToString(const int32_t& value) { char str[32]; sprintf(str, "%i", value); return std::string(str); }
    template<> std::string ToString(const uint64_t& value) { char str[32]; sprintf(str, "%llu", value); return std::string(str); }
    template<> std::string ToString(const int64_t& value) { char str[32]; sprintf(str, "%lli", value); return std::string(str); }
    template<> std::string ToString(const float& value) { char str[32]; sprintf(str, "%fg", value); return std::string(str); }
    template<> std::string ToString(const bool& value) { char str[32]; sprintf(str, "%u", (uint8_t)value); return std::string(str); }    
}
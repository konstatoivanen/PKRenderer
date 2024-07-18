#pragma once
namespace PK::Parse
{
    template<typename T>
    T FromString(const char* str);

    template<typename T>
    std::string ToString(const T& value);

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
}

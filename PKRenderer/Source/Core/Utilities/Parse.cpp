#include "PrecompiledHeader.h"
#include "FixedString.h"
#include "Parse.h"

namespace PK::Parse
{
    FixedString32 BytesToString(size_t bytes)
    {
        if (bytes == 0)
        {
            return FixedString32("0B");
        }

        auto mag = (int)(log(bytes) / log(1024));
        mag = glm::min(mag, 2);

        auto adjustedSize = (double)bytes / (1L << (mag * 10));
        auto factor = pow(10, 4);

        if ((round(adjustedSize * factor) / factor) >= 1000)
        {
            mag += 1;
            adjustedSize /= 1024;
        }

        char buffer[64];

        auto length = snprintf(buffer, 61u, "%1.4g", adjustedSize);

        switch (mag)
        {
            case 0: buffer[length + 0u] = 'B'; buffer[length + 1u] = '\0'; break;
            case 1: buffer[length + 0u] = 'K'; buffer[length + 1u] = 'B'; buffer[length + 2u] = '\0'; break;
            case 2: buffer[length + 0u] = 'M'; buffer[length + 1u] = 'B'; buffer[length + 2u] = '\0'; break;
            default: buffer[length + 0u] = 'G'; buffer[length + 1u] = 'B'; buffer[length + 2u] = '\0'; break;
        }

        return FixedString32(length + 2u, buffer);
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
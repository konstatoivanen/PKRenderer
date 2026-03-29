#pragma once
#include <stdint.h>
#include <string.h>

namespace PK
{
    struct CArguments
    {
        char** args;
        int count;
    };

    struct CArgument
    {
        char* arg;
    };

    struct CArgumentsConst
    {
        const char* const* args;
        int count;
    };

    struct CArgumentConst
    {
        const char* arg;
    };

    template<uint32_t capacity, uint32_t maxArguments>
    struct CArgumentsInline
    {
        char argumentData[capacity];
        char* arguments[maxArguments];
        uint32_t count;

        CArgumentsInline() : count(0u)
        {
            arguments[0] = argumentData;
            argumentData[0] = '\0';
            argumentData[capacity - 1u] = '\0';
        }

        CArgumentsInline(const char* arg) : CArgumentsInline()
        {
            if (arg && arg[0])
            {
                strncpy(argumentData, arg, capacity - 1u);
                count = 1u;
            }
        }

        CArgumentsInline(const char* arg, char separator) : CArgumentsInline()
        {
            if (arg && arg[0])
            {
                auto i = 0u;
                count = 1u;
                argumentData[i++] = arg[0];

                for (; i < capacity - 1ull && arg[i]; ++i)
                {
                    argumentData[i] = arg[i];

                    if (arg[i] == separator)
                    {
                        argumentData[i] = '\0';
                    }
                    else if (arg[i - 1u] == separator)
                    {
                        arguments[count++] = argumentData + i;
                    }
                }

                argumentData[i] = '\0';
            }
        }

        CArgumentsInline(const char* const* args, uint32_t count) : CArgumentsInline()
        {
            auto head = 0ull;

            for (auto i = 0u; i < count && i < maxArguments && args[i] && args[i][0] && head < capacity; ++i)
            {
                arguments[this->count++] = argumentData + head;

                for (auto j = 0u; j < capacity && args[i][j]; ++j)
                {
                    if (head < capacity - 1ull)
                    {
                        argumentData[head++] = args[i][j];
                    }
                }

                argumentData[head++] = '\0';
            }
        }
    };

    typedef CArgumentsInline<512, 16> CArgumentsInlineDefault;
}

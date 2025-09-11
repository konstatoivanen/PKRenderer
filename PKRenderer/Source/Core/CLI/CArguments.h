#pragma once
#include <exception>

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
        }

        CArgumentsInline(const char* arg)
        {
            auto length = strlen(arg) + 1u;

            if (length >= capacity)
            {
                throw std::exception("Arguments size exceeds fixed capacity!");
            }

            arguments[0] = &argumentData;
            memcpy(argumentData, arg, length);
            count = 1u;
        }

        CArgumentsInline(const char* argument, char separator)
        {
            auto length = strlen(argument);

            if (length + 1u >= capacity)
            {
                throw std::exception("Arguments size exceeds fixed capacity !");
            }

            memcpy(argumentData, argument, length + 1u);

            count = 0u;

            for (auto i = 0u; i < length; ++i)
            {
                if (argumentData[i] == separator)
                {
                    argumentData[i] = '\0';
                }

                if (i == 0 || argumentData[i - 1u] == '\0')
                {
                    arguments[count++] = argumentData + i;
                }
            }
        }

        CArgumentsInline(const char* const* args, uint32_t count)
        {
            size_t head = 0ull;

            for (auto i = 0u; i < count; ++i)
            {
                head += strlen(args[i]) + 1ull;
            }

            if (head >= capacity)
            {
                throw std::exception("Arguments size exceeds fixed capacity!");
            }

            if (count >= maxArguments)
            {
                throw std::exception("Argument count exceeds fixed capacity!");
            }

            head = 0ull;

            for (auto i = 0u; i < count; ++i)
            {
                auto length = strlen(args[i]) + 1ull;
                memcpy(argumentData + head, args[i], length);
                arguments[i] = argumentData + head;
                head += length;
            }

            this->count = count;
        }
    };

    typedef CArgumentsInline<512, 16> CArgumentsInlineDefault;
}

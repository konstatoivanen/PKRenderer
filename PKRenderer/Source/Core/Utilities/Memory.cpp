#include "PrecompiledHeader.h"
#include <stdlib.h>
#include <exception>
#include "Memory.h"

namespace PK
{
    void Memory::Assert(bool value, const char* str)
    {
        if (!value)
        {
            PK_PLATFORM_DEBUG_BREAK;
            throw std::exception(str);
        }
    }

    void Memory::Assert(bool value)
    {
        Assert(value, "Assertion failed!");
    }
}

#include "PrecompiledHeader.h"
#include <exception>
#include "Memory.h"

namespace PK
{
    void Memory::Assert(bool value)
    {
        if (!value)
        {
            PK_PLATFORM_DEBUG_BREAK;
            throw std::exception("Assertion failed!");
        }
    }
}

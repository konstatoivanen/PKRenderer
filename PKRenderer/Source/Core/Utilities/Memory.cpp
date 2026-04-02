#include "PrecompiledHeader.h"
#include <stdlib.h>
#include "Memory.h"

namespace PK
{
    void Memory::Assert(bool value, const char* str)
    {
        if (!value)
        {
            PK::Platform::FatalExit(str);
        }
    }

    void Memory::Assert(bool value)
    {
        Assert(value, "Assertion failed!");
    }
}

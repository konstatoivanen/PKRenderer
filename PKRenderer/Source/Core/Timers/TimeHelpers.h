#pragma once
#include <stdint.h>

namespace PK
{
    namespace TimeHelpers
    {
        uint64_t GetClockTicks();
        double GetClockSeconds();
        double GetSteadySeconds();
    }
}

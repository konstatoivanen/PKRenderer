#include "PrecompiledHeader.h"
#include <ctime>
#include "TimeHelpers.h"

namespace PK
{
    uint64_t TimeHelpers::GetClockTicks() { return (uint64_t)clock(); }
    double TimeHelpers::GetClockSeconds() { return clock() / (double)CLOCKS_PER_SEC; }
    double TimeHelpers::GetSteadySeconds() { return Platform::GetTimeSeconds(); }
}

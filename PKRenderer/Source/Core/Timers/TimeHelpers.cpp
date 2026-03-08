#include "PrecompiledHeader.h"
#include <ctime>
#include <chrono>
#include "TimeHelpers.h"

namespace PK
{
    typedef std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> TimePointSeconds;
    uint64_t TimeHelpers::GetClockTicks() { return (uint64_t)clock(); }
    double TimeHelpers::GetClockSeconds() { return clock() / (double)CLOCKS_PER_SEC; }
    double TimeHelpers::GetSteadySeconds() { return TimePointSeconds(std::chrono::steady_clock::now()).time_since_epoch().count(); }
}

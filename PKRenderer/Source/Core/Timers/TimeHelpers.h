#pragma once
#include <ctime>
#include <chrono>

namespace PK
{
    typedef std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> TimePointSeconds;

    namespace TimeHelpers
    {
        inline clock_t GetClockTicks() { return clock(); }
        inline double GetClockSeconds() { return clock() / (double)CLOCKS_PER_SEC; }
        inline TimePointSeconds GetTimePointSecondsNow() { return std::chrono::steady_clock::now(); }
    }
}
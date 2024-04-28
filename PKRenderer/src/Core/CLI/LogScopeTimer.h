#pragma once
#include "Utilities/NoCopy.h"
#include <chrono>

namespace PK::Core::CLI
{
    struct LogScopeTimer : public Utilities::NoCopy
    {
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> start;
        const char* name;
        LogScopeTimer(const char* name);
        ~LogScopeTimer();
    };
}
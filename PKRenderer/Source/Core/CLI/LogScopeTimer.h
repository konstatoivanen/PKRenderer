#pragma once
#include <chrono>
#include "Core/Utilities/NoCopy.h"

namespace PK
{
    struct LogScopeTimer : public NoCopy
    {
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> start;
        const char* name;
        LogScopeTimer(const char* name);
        ~LogScopeTimer();
    };
}
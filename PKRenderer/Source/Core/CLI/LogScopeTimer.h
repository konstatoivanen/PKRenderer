#pragma once
#include <chrono>
#include "Core/Utilities/NoCopy.h"

namespace PK
{
    struct LogScopeTimer : public NoCopy
    {
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> start;
        const char* name;
        size_t length;
        LogScopeTimer(const char* name);
        LogScopeTimer(size_t length, const char* name);
        ~LogScopeTimer();
    };
}
#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "LogScopeTimer.h"

namespace PK
{
    LogScopeTimer::LogScopeTimer(const char* name) :
        start(std::chrono::steady_clock::now()),
        name(name),
        length(strlen(name))
    {
    }

    LogScopeTimer::LogScopeTimer(size_t length, const char* name) :
        start(std::chrono::steady_clock::now()),
        name(name),
        length(length)
    {
    }

    LogScopeTimer::~LogScopeTimer()
    {
        std::chrono::time_point<std::chrono::steady_clock, std::chrono::duration<double>> end = std::chrono::steady_clock::now();
        auto delta = (end - start) * 1000.0;
        StaticLog::Log(LogSeverity::PK_LOG_LVL_INFO, LogColor::PK_LOG_COLOR_INFO, "ScopeTimer: %.*s, %4.4f ms", (int32_t)length, name, delta);
    }
}
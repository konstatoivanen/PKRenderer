#include "PrecompiledHeader.h"
#include "Core/Timers/TimeHelpers.h"
#include "Core/CLI/Log.h"
#include "LogScopeTimer.h"

namespace PK
{
    LogScopeTimer::LogScopeTimer(const char* name) :
        start(TimeHelpers::GetSteadySeconds()),
        name(name),
        length(strlen(name))
    {
    }

    LogScopeTimer::LogScopeTimer(size_t length, const char* name) :
        start(TimeHelpers::GetSteadySeconds()),
        name(name),
        length(length)
    {
    }

    LogScopeTimer::~LogScopeTimer()
    {
        auto delta = (TimeHelpers::GetSteadySeconds() - start) * 1000.0;
        StaticLog::Log(LogSeverity::PK_LOG_LVL_INFO, LogColor::PK_LOG_COLOR_INFO, "ScopeTimer: %.*s, %4.4f ms", (int32_t)length, name, delta);
    }

    LogScopeTimeAggregator::LogScopeTimeAggregator(volatile State* state, const char* name) :
        state(state),
        start(TimeHelpers::GetSteadySeconds()),
        name(name),
        length(strlen(name))
    {
    }

    LogScopeTimeAggregator::LogScopeTimeAggregator(volatile State* state, size_t length, const char* name) :
        state(state),
        start(TimeHelpers::GetSteadySeconds()),
        name(name),
        length(length)
    {
    }

    LogScopeTimeAggregator::~LogScopeTimeAggregator()
    {
        auto delta = (TimeHelpers::GetSteadySeconds() - start);
        state->elapsed += delta;
        state->ticks++;
        delta = (state->elapsed / state->ticks) * 1000.0;
        StaticLog::Log(LogSeverity::PK_LOG_LVL_INFO, LogColor::PK_LOG_COLOR_INFO, "ScopeTimer Avg: %.*s, %4.4f ms, %ull ticks", (int32_t)length, name, delta, state->ticks);
    }
}

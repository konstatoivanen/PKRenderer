#pragma once
#include "Core/Utilities/NoCopy.h"

namespace PK
{
    struct LogScopeTimer : public NoCopy
    {
        double start;
        const char* name;
        size_t length;
        LogScopeTimer(const char* name);
        LogScopeTimer(size_t length, const char* name);
        ~LogScopeTimer();
    };


    struct LogScopeTimeAggregator : public NoCopy
    {
        struct State
        {
            double elapsed = 0.0;
            size_t ticks = 0ull;
        };

        volatile State* state;
        double start;
        const char* name;
        size_t length;
        LogScopeTimeAggregator(volatile State* state, const char* name);
        LogScopeTimeAggregator(volatile State* state, size_t length, const char* name);
        ~LogScopeTimeAggregator();
    };
}

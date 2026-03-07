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
}

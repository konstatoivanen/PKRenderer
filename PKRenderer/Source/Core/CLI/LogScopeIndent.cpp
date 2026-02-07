#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "LogScopeIndent.h"

namespace PK
{
    LogScopeIndent::LogScopeIndent(unsigned int severity) : severity(severity)
    {
        StaticLog::Indent((LogSeverity)severity);
    }

    LogScopeIndent::~LogScopeIndent()
    {
        StaticLog::Outdent((LogSeverity)severity);
    }
}

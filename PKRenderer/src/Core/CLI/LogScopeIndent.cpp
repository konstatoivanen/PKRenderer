#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "LogScopeIndent.h"

namespace PK::Core::CLI
{
    LogScopeIndent::LogScopeIndent()
    {
        StaticLog::Indent();
    }

    LogScopeIndent::~LogScopeIndent()
    {
        StaticLog::Unindent();
    }
}
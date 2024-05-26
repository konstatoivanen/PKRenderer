#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "LogScopeIndent.h"

namespace PK
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
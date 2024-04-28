#pragma once
#include "Utilities/NoCopy.h"

namespace PK::Core::CLI
{
    struct LogScopeIndent : public Utilities::NoCopy
    {
        LogScopeIndent();
        ~LogScopeIndent();
    };
}
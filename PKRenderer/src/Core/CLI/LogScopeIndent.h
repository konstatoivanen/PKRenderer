#pragma once
#include "Core/Utilities/NoCopy.h"

namespace PK
{
    struct LogScopeIndent : public NoCopy
    {
        LogScopeIndent();
        ~LogScopeIndent();
    };
}
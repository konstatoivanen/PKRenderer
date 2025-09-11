#pragma once
#include "Core/Utilities/NoCopy.h"

namespace PK
{
    struct LogScopeIndent : public NoCopy
    {
        unsigned int severity;
        LogScopeIndent(unsigned int severity = 0xFFFFFFFFu);
        ~LogScopeIndent();
    };
}

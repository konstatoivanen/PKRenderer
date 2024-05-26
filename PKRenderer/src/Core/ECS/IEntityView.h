#pragma once
#include "Core/ECS/EGID.h"

namespace PK
{
    struct IEntityView
    {
        EGID GID;
        virtual ~IEntityView() = default;
    };
}

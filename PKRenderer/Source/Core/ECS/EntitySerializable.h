#pragma once
#include "Core/Utilities/UUID128.h"
#include "Core/Utilities/FixedString.h"

namespace PK
{
    // Entity view serialization tracker.
    struct EntityViewSerializable
    {
        FixedString64 name;
        UUID128 typeUUID;
    };
}

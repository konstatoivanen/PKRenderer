#pragma once
#include "Core/Base/Containers/FixedString.h"
#include "Core/Base/Types/UUID128.h"

namespace PK
{
    // Entity view serialization tracker.
    struct EntityViewSerializable
    {
        UUID128 typeUUID;
        FixedString64 name;
    };
}

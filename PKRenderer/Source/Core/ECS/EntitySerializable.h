#pragma once
#include "Core/Utilities/UUID128.h"
#include "Core/Utilities/FixedString.h"

namespace PK
{
    enum class EntitySerializationFlags : uint64_t
    {
        // Entity serialization is dependent on some parent entity.
        Child = 1u << 0u, 
        // Entity will be serialized with a fixed identifier. Crashes if the identifier conflicts with another identifier.
        FixedIdentifier = 1u << 1u, 
    };

    // Entity view serialization tracker.
    struct EntityViewSerializable
    {
        FixedString64 name;
        UUID128 typeUUID;
        EntitySerializationFlags flags;
    };
}

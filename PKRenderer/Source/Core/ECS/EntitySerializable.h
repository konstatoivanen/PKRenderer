#pragma once
#include "Core/Base/Containers/FixedString.h"
#include "Core/Base/Types/UUID128.h"

// @TODO forward declare these somewhere
namespace c4::yml
{
    class NodeRef;
    class ConstNodeRef;
}

namespace ryml 
{
    using namespace c4::yml;
    using namespace c4;
}

namespace PK
{
    typedef ryml::ConstNodeRef SerialNodeRead;
    typedef ryml::NodeRef SerialNodeWrite;
    struct EntityDatabase;

    // Entity view serialization tracker.
    struct ComponentSerializable
    {
        UUID128 typeUUID;
        FixedString64 name;
    };

    struct EntityViewSerializable
    {
        uint32_t* entityId;
        ComponentSerializable* serializable;
    };

    struct EntitySerializer
    {
        UUID128 uuid;
        const char* name;
        void (*serialize)(EntityDatabase*, SerialNodeWrite, const uint32_t);
        uint32_t(*deserialize)(EntityDatabase*, SerialNodeRead, const char*);

        constexpr bool operator == (const EntitySerializer& r) const noexcept
        {
            return uuid == r.uuid;
        }

        struct SerializerHash
        {
            constexpr size_t operator()(const UUID128& k) const noexcept
            {
                return k.low;
            }
        };
    };
}

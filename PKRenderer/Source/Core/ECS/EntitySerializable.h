#pragma once
#include "Core/Base/Containers/FixedString.h"
#include "Core/Base/Types/UUID128.h"
#include "EGID.h"

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
    struct EntityViewSerializable
    {
        UUID128 typeUUID;
        FixedString64 name;
    };

    struct EntitySerializer
    {
        UUID128 uuid;
        const char* name;
        void (*serialize)(EntityDatabase*, SerialNodeWrite, const EGID&);
        EGID(*deserialize)(EntityDatabase*, SerialNodeRead, uint32_t, const char*);

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

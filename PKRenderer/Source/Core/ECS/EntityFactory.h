#pragma once
#include "Core/ECS/EGID.h"
#include "Core/ECS/EntityDatabase.h"
#include "Core/Serialization/Serialize.h"

namespace PK
{
    struct EntityDatabase;

    template<typename TEntity>
    struct EntityFactory
    {
        static EGID Create(EntityDatabase* entityDb, EGID egid, const TEntity& descriptor);
        static EGID CreateDefault(EntityDatabase* entityDb, EGID egid);
        static EGID Deserialize(EntityDatabase* entityDb, SerialNodeRead parent, uint32_t group);
        static void Serialize(EntityDatabase* entityDb, SerialNodeWrite parent, const EGID& egid);

        inline static EGID Create(EntityDatabase* entityDb, uint32_t groupId, const TEntity& descriptor)
        {
            return Create(entityDb, EGID(0u, groupId), descriptor);
        }

        inline static EGID CreateDefault(EntityDatabase* entityDb, uint32_t groupId)
        {
            return CreateDefault(entityDb, EGID(0u, groupId));
        }
    };
}

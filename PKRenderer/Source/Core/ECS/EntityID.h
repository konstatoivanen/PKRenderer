#pragma once
#include <stdint.h>

namespace PK
{
    struct EntityID
    {
        uint64_t identifier = 0ull;

        EntityID(uint32_t entityId, uint32_t compositionIndex, uint32_t arrayIndex)
        {
            identifier |= ((uint64_t)entityId & 0xFFFFFFull) << 0ull;
            identifier |= ((uint64_t)compositionIndex & 0xFFFFull) << 24ull;
            identifier |= ((uint64_t)arrayIndex & 0xFFFFFFull) << 48ull;
        }

        bool operator == (const EntityID& other) { return entityId() == other.entityId(); }
        bool operator != (const EntityID& other) { return entityId() != other.entityId(); }
        uint32_t entityId() const { return identifier & 0xFFFFFFull; }
        uint32_t compositionIndex() const { return (identifier >> 24ull) & 0xFFFFull; }
        uint32_t arrayIndex() const { return (identifier >> 48ull) & 0xFFFFFFull; }
    };

    struct EntityIDHash
    {
        size_t operator()(const EntityID& k) const noexcept { return k.entityId(); }
    };
}
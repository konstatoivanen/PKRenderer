#pragma once
#include "Core/Base/Containers/FixedString.h"

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

    // 64 bit for alignment reasons.
    enum class EntitySerialFlags : uint64_t
    {
        None = 0ull,
        Serialize = 1ull << 0ull
    };

    inline constexpr EntitySerialFlags operator|(EntitySerialFlags a, EntitySerialFlags b) noexcept { return (EntitySerialFlags)((uint32_t)a | (uint32_t)b); }
    inline constexpr EntitySerialFlags operator&(EntitySerialFlags a, EntitySerialFlags b) noexcept { return (EntitySerialFlags)((uint32_t)a & (uint32_t)b); }
    inline constexpr EntitySerialFlags& operator|=(EntitySerialFlags& a, EntitySerialFlags b) noexcept { return a = a | b; }
    inline constexpr EntitySerialFlags operator&(EntitySerialFlags a, uint32_t b) noexcept { return (EntitySerialFlags)((uint32_t)a & b); }
    inline constexpr bool operator == (const EntitySerialFlags& a, const uint32_t& b) noexcept { return (uint32_t)a == b; }
    inline constexpr bool operator != (const EntitySerialFlags& a, const uint32_t& b) noexcept { return (uint32_t)a != b; }

    // Entity view serialization tracker.
    struct ComponentSerializable
    {
        FixedString64 name;
        EntitySerialFlags flags;
    };

    struct EntityViewSerializable
    {
        uint32_t* entityId;
        ComponentSerializable* serializable;
    };
}

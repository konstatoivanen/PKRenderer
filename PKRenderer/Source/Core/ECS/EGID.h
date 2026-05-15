#pragma once
#include <stdint.h>

namespace PK
{
    struct EGID
    {
        constexpr uint32_t entityID() const { return (uint32_t)(identifier & 0xFFFFFFFF); }
        constexpr uint32_t groupID() const { return (uint32_t)(identifier >> 32); }
        constexpr EGID() : identifier(0) {}
        constexpr EGID(const EGID& other) : identifier(other.identifier) {}
        constexpr EGID(uint64_t identifier) : identifier(identifier) {}
        constexpr EGID(uint32_t entityID, uint32_t groupID) : identifier((uint64_t)groupID << 32 | ((uint64_t)(uint32_t)entityID & 0xFFFFFFFF)) {}
        constexpr bool IsValid() const { return identifier > 0; }
        constexpr bool operator ==(const EGID& obj2) const { return identifier == obj2.identifier; }
        constexpr bool operator !=(const EGID& obj2) const { return identifier != obj2.identifier; }
        constexpr bool operator <(const EGID& obj2) const { return identifier < obj2.identifier; }
        constexpr bool operator >(const EGID& obj2) const { return identifier > obj2.identifier; }
        uint64_t identifier;
    };

    constexpr static const EGID EGIDDefault = EGID(1);
    constexpr static const EGID EGIDInvalid = EGID(0);
}

#pragma once
#include <cstdint>

namespace PK
{
    struct EGID
    {
    public:
        constexpr uint32_t entityID() const { return (uint32_t)(m_GID & 0xFFFFFFFF); }
        constexpr uint32_t groupID() const { return (uint32_t)(m_GID >> 32); }
        constexpr EGID() : m_GID(0) {}
        constexpr EGID(const EGID& other) : m_GID(other.m_GID) {}
        constexpr EGID(uint64_t identifier) : m_GID(identifier) {}
        constexpr EGID(uint32_t entityID, uint32_t groupID) : m_GID((uint64_t)groupID << 32 | ((uint64_t)(uint32_t)entityID & 0xFFFFFFFF)) {}
        constexpr bool IsValid() const { return m_GID > 0; }
        constexpr bool operator ==(const EGID& obj2) const { return m_GID == obj2.m_GID; }
        constexpr bool operator !=(const EGID& obj2) const { return m_GID != obj2.m_GID; }
        constexpr bool operator <(const EGID& obj2) const { return m_GID < obj2.m_GID; }
        constexpr bool operator >(const EGID& obj2) const { return m_GID > obj2.m_GID; }

    private:
        uint64_t m_GID;
    };

    constexpr static const EGID EGIDDefault = EGID(1);
    constexpr static const EGID EGIDInvalid = EGID(0);
}
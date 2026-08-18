#pragma once
#include <stdint.h>

namespace PK::App
{
    enum class ScenePrimitiveFlags : uint8_t
    {
        None = 0u,
        Mesh = 1 << 0,
        Light = 1 << 1,
        CastShadows = 1 << 2,
        NeverCull = 1 << 3,
        RayTraceable = 1 << 4,

        // Presets
        DefaultMesh = Mesh | CastShadows | RayTraceable,
        DefaultLight = Light | CastShadows
    };

    enum class LightType : uint8_t
    {
        Directional = 0,
        Spot = 1,
        Point = 2,
        TypeCount
    };

    inline constexpr ScenePrimitiveFlags operator|(ScenePrimitiveFlags a, ScenePrimitiveFlags b) noexcept { return (ScenePrimitiveFlags)((uint32_t)a | (uint32_t)b); }
    inline constexpr ScenePrimitiveFlags operator&(ScenePrimitiveFlags a, ScenePrimitiveFlags b) noexcept { return (ScenePrimitiveFlags)((uint32_t)a & (uint32_t)b); }
    inline constexpr ScenePrimitiveFlags& operator|=(ScenePrimitiveFlags& a, ScenePrimitiveFlags b) noexcept { return a = a | b; }
    inline constexpr ScenePrimitiveFlags operator&(ScenePrimitiveFlags a, uint32_t b) noexcept { return (ScenePrimitiveFlags)((uint32_t)a & b); }
    inline constexpr bool operator == (const ScenePrimitiveFlags& a, const uint32_t& b) noexcept { return (uint32_t)a == b; }
    inline constexpr bool operator != (const ScenePrimitiveFlags& a, const uint32_t& b) noexcept { return (uint32_t)a != b; }
}

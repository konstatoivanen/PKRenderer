#pragma once

namespace PK::Rendering::Structs
{
    enum class RenderableFlags : uint8_t
    {
        Mesh = 1 << 0,
        Light = 1 << 1,
        Static = 1 << 2,
        CastShadows = 1 << 3,
        Cullable = 1 << 4,
        RayTraceable = 1 << 5,

        // Presets
        DefaultMesh = Mesh | Static | CastShadows | Cullable | RayTraceable,
        DefaultMeshNoShadows = Mesh | Static | Cullable,
    };

    enum class LightType : uint8_t
    {
        Directional = 0,
        Spot = 1,
        Point = 2,
        TypeCount
    };

    enum class Cookie : uint8_t
    {
        Circle0,
        Circle1,
        Circle2,
        Square0,
        Square1,
        Square2,
        Triangle,
        Star,
    };

    #define PK_DECLARE_ENUM_OPERATORS(Type) \
    static constexpr Type operator | (const Type& a, const Type& b) { return (Type)((uint32_t)a | (uint32_t)b); } \
    static constexpr Type operator |= (const Type& a, const Type& b) { return a | b; } \
    static constexpr Type operator & (const Type& a, const Type& b) { return (Type)((uint32_t)a & (uint32_t)b); } \
    static constexpr Type operator & (const Type& a, const int& b) { return (Type)((uint32_t)a & (uint32_t)b); } \
    static constexpr bool operator == (const Type& a, const int& b) { return (uint32_t)a == b; } \
    static constexpr bool operator != (const Type& a, const int& b) { return (uint32_t)a != b; } \

    PK_DECLARE_ENUM_OPERATORS(RenderableFlags)

    #undef PK_DECLARE_ENUM_OPERATORS
}
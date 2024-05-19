#pragma once
#include <cstdint>

namespace PK::Renderer
{
    enum class ScenePrimitiveFlags : uint8_t
    {
        None = 0u,
        Mesh = 1 << 0,
        Light = 1 << 1,
        Static = 1 << 2,
        CastShadows = 1 << 3,
        NeverCull = 1 << 4,
        RayTraceable = 1 << 5,

        // Presets
        DefaultMesh = Mesh | Static | CastShadows | RayTraceable,
        DefaultMeshNoShadows = Mesh | Static,
    };

    enum class RenderViewType : uint8_t
    {
        Scene,
        MaterialPreview,
        MeshPreview,
        GUIWindow,
        Count
    };

    constexpr static const char* RenderViewTypeName[(int)RenderViewType::Count] =
    {
        "Scene",
        "MaterialPreview",
        "MeshPreview",
        "GUIWindow"
    };

    enum class LightType : uint8_t
    {
        Directional = 0,
        Spot = 1,
        Point = 2,
        TypeCount
    };

    enum class LightCookie : uint8_t
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

    static constexpr ScenePrimitiveFlags operator | (const ScenePrimitiveFlags& a, const ScenePrimitiveFlags& b) { return (ScenePrimitiveFlags)((uint32_t)a | (uint32_t)b); }
    static constexpr ScenePrimitiveFlags operator |= (const ScenePrimitiveFlags& a, const ScenePrimitiveFlags& b) { return a | b; }
    static constexpr ScenePrimitiveFlags operator & (const ScenePrimitiveFlags& a, const ScenePrimitiveFlags& b) { return (ScenePrimitiveFlags)((uint32_t)a & (uint32_t)b); }
    static constexpr ScenePrimitiveFlags operator & (const ScenePrimitiveFlags& a, const int& b) { return (ScenePrimitiveFlags)((uint32_t)a & (uint32_t)b); }
    static constexpr bool operator == (const ScenePrimitiveFlags& a, const int& b) { return (uint32_t)a == b; }
    static constexpr bool operator != (const ScenePrimitiveFlags& a, const int& b) { return (uint32_t)a != b; }
}

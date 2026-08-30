#pragma once
#include "Core/Math/Math.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    // Required render modes and texture indices.
    constexpr static const uint16_t GUI_TEX_INDEX_WHITE = 0u;
    constexpr static const uint16_t GUI_TEX_INDEX_ERROR = 1u;
    constexpr static const uint16_t GUI_TEX_INDEX_DEFAULT_FONT = 2u;
    constexpr static const uint16_t GUI_RENDER_MODE_DEFAULT = 0u;
    constexpr static const uint16_t GUI_RENDER_MODE_TEXT = 1u;

    #define GUI_USE_16BIT_INDICES 1

    #if GUI_USE_16BIT_INDICES
    using GUIIndex = uint16_t;
    #else 
    using GUIIndex = uint32_t;
    #endif

    struct GUIVertex
    {
        color32 color;
        short2 coord;
        ushort2 texcoordHalf;
        uint16_t textureIndex;
        uint16_t renderMode;
    };

    struct GUIAllocation
    {
        GUIVertex* vertices = nullptr;
        GUIIndex* indices = nullptr;
        uint32_t vertexOffset = 0u;
        uint32_t vertexCount = 0u;
        uint32_t indexCount = 0u;
        uint32_t layer = 0u;
    };

    struct IGUIAllocator
    {
        virtual ~IGUIAllocator() = default;
        virtual short4 GUIGetRenderAreaRect() const = 0;
        virtual Font* GUIGetDefaultFont() const = 0;
        virtual uint16_t GUIGetTextureIndex(RHITexture* texture) = 0;
        virtual uint3 GUIGetTextureSize(uint16_t textureIndex) const = 0;
        virtual bool GUIAllocate(uint32_t layer, uint32_t vertexCount, uint32_t indexCount, GUIAllocation* allocation) = 0;
    };
}

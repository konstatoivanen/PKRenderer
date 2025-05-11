#pragma once
#include "Core/Math/Math.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK::App
{
    struct GUIVertex
    {
        color32 color;
        short2 coord;
        ushort2 texcoordHalf;
        uint16_t textureIndex;
        uint16_t renderMode;
    };

    struct IGUIRenderer
    {
        virtual ~IGUIRenderer() = default;

        virtual short4 GUIGetRenderAreaRect() const = 0;
        virtual uint16_t GUIAddTexture(RHITexture* texture) = 0;
        virtual void GUIDrawTriangle(const GUIVertex& a, const GUIVertex& b, const GUIVertex& c) = 0;
        virtual void GUIDrawRect(const color32& color, const short4& rect, const ushort4& textureRect, RHITexture* texture) = 0;
        virtual void GUIDrawRect(const color32& color, const short4& rect, const ushort4& textureRect, uint16_t textureIndex) = 0;
        virtual void GUIDrawRect(const color32& color, const short4& rect) = 0;
        virtual void GUIDrawWireRect(const color32& color, const short4& rect, short inset) = 0;
        virtual void GUIDrawText(const color32& color, const short2& coord, const char* text, TextAlign alignx, TextAlign aligny, float size = 1.0f, float lineSpacing = 1.0f) = 0;
        virtual void GUIDrawText(const color32& color, const short2& coord, const char* text, float size = 1.0f, float lineSpacing = 1.0f) = 0;
    };

    struct IGizmosRenderer
    {
        virtual void GizmosDrawBounds(const BoundingBox& aabb) = 0;
        virtual void GizmosDrawBox(const float3& origin, const float3& size) = 0;
        virtual void GizmosDrawLine(const float3& start, const float3& end) = 0;
        virtual void GizmosDrawRay(const float3& origin, const float3& vector) = 0;
        virtual void GizmosDrawFrustrum(const float4x4& matrix) = 0;
        virtual void GizmosSetColor(const color& color) = 0;
        virtual void GizmosSetMatrix(const float4x4& matrix) = 0;
    };

    struct GUICombinedRenderEvent
    {
        IGUIRenderer* gui;
        IGizmosRenderer* gizmos;
    };
}
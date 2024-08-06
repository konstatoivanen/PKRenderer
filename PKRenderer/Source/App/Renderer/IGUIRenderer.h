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

    // Windows.h defines DrawText macro. lets undef that.
    #ifdef DrawText
    #undef DrawText
    #endif

    struct IGUIRenderer
    {
        virtual ~IGUIRenderer() = default;

        virtual short4 GetRenderAreaRect() const = 0;
        virtual uint16_t AddTexture(RHITexture* texture) = 0;
        virtual void DrawTriangle(const GUIVertex& a, const GUIVertex& b, const GUIVertex& c) = 0;
        virtual void DrawRect(const color32& color, const short4& rect, const ushort4& textureRect, RHITexture* texture) = 0;
        virtual void DrawRect(const color32& color, const short4& rect, const ushort4& textureRect, uint16_t textureIndex) = 0;
        virtual void DrawRect(const color32& color, const short4& rect) = 0;
        virtual void DrawWireRect(const color32& color, const short4& rect, short inset) = 0;
        virtual void DrawText(const color32& color, const short2& coord, const char* text, TextAlign alignx, TextAlign aligny, float size = 1.0f, float lineSpacing = 1.0f) = 0;
        virtual void DrawText(const color32& color, const short2& coord, const char* text, float size = 1.0f, float lineSpacing = 1.0f) = 0;
    };
}
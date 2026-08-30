#pragma once
#include "Core/Base/Containers/ArrayList.h"
#include "IGUIAllocator.h"

namespace PK
{
    struct GUIDrawList
    {
        GUIDrawList(IGUIAllocator* allocator) : m_allocator(allocator) {}

        void PushClipRect(const short4& rect);
        void PopClipRect();

        void PushLayer();
        void PopLayer();

        short4 GetClipRect() const;
        uint16_t GetLayer() const;

        void DrawTriangle(const GUIVertex& a, const GUIVertex& b, const GUIVertex& c);
        void DrawLine(const short2& p0, const short2& p1, const color32& color0, const color32& color1, const float width);
        void DrawLine(const short2& p0, const short2& p1, const color32& color, const float width);
        void DrawCurve(const short2* points, const color32& color0, const color32& color1, uint32_t count, const float width);
        void DrawCurve(const short2* points, const color32& color, uint32_t count, const float width);
        void DrawBezierCurve(const short2& p0, const short2& p1, const short2& cp0, const short2& cp1, const color32& color0, const color32& color1, float width, float density = 12.0f);
        void DrawBezierCurve(const short2& p0, const short2& p1, const short2& cp0, const short2& cp1, const color32& color, float width, float density = 12.0f);
        
        void DrawRect(const color32& color, const short4& rect, const ushort4& textureRect, uint16_t textureIndex, uint16_t renderMode);
        void DrawRect(const color32& color, const short4& rect, const ushort4& textureRect, uint16_t textureIndex);
        void DrawRect(const color32& color, const short4& rect, const ushort4& textureRect, RHITexture* texture, uint16_t renderMode);
        void DrawRect(const color32& color, const short4& rect, const ushort4& textureRect, RHITexture* texture);
        void DrawRect(const color32& color, const short4& rect, uint16_t renderMode);
        void DrawRect(const color32& color, const short4& rect);

        void DrawWireRect(const color32& color, const short4& rect, short inset, uint16_t renderMode);
        void DrawWireRect(const color32& color, const short4& rect, short inset);

        FontGeometryInfo CalculateText(const short4& rect, const char* text, Font* font, const FontStyle& style);
        FontGeometryInfo CalculateText(const short4& rect, const char* text, const FontStyle& style);
        short4 DrawText(const color32& color, const FontGeometryInfo& info);
        short4 DrawText(const color32& color, const short4& rect, const char* text, Font* font, const FontStyle& style);
        short4 DrawText(const color32& color, const short4& rect, const char* text, const FontStyle& style);

    private:
        IGUIAllocator* m_allocator = nullptr;
        InlineList<short4, 8u> m_clipStack;
        uint16_t m_layer = 0u;
    };
}

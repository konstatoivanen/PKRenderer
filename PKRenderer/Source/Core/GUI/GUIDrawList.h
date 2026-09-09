#pragma once
#include "IGUIAllocator.h"
#include "Core/Rendering/Font.h"

namespace PK
{
    struct GUIDrawList
    {
        constexpr const static uint32_t CLIP_STACK_MAX = 15u;

        GUIDrawList(IGUIAllocator* allocator, const short4& initialClipRect);

        void PushClipRect(const short4& rect);
        void PopClipRect();
        short4 GetClipRect() const;

        void PushLayer();
        void PopLayer();
        uint16_t GetLayer() const;
        
        void Triangle(const GUIVertex& a, const GUIVertex& b, const GUIVertex& c);
        void Line(const short2& p0, const short2& p1, const color32& color0, const color32& color1, const float width);
        void Curve(const short2* points, const color32& color0, const color32& color1, uint32_t count, const float width);
        void BezierCurve(const short2& p0, const short2& p1, const short2& cp0, const short2& cp1, const color32& color0, const color32& color1, float width, float density = 12.0f);

        void Rect(const color32& color, const short4& rect, const ushort4& textureRect, uint16_t textureIndex, uint16_t renderMode);
        void Rect(const color32& color, const short4& rect, const ushort4& textureRect, RHITexture* texture, uint16_t renderMode);

        void WireRect(const color32& color, const short4& rect, short inset);
        void DentedRect(const color32& color, const short4& rect, short2 dent);
        void DentedWireRect(const color32& color, const short4& rect, short inset, short2 dent);
        void X(const color32& color, const short4& rect, short thickness);

        void LowerRightTriangle(const color32& color, const short4& rect);
        void LowerLeftTriangle(const color32& color, const short4& rect);
        void UpperRightTriangle(const color32& color, const short4& rect);
        void UpperLeftTriangle(const color32& color, const short4& rect);

        void LowerRightWedge(const color32& color, const short4& rect, short thickness);
        void LowerLeftWedge(const color32& color, const short4& rect, short thickness);
        //void UpperRightWedge(const color32& color, const short4& rect, short thickness);
        //void UpperLeftWedge(const color32& color, const short4& rect, short thickness);

        void RightWedge(const color32& color, const short4& rect, short thickness);
        void LeftWedge(const color32& color, const short4& rect, short thickness);
        void UpWedge(const color32& color, const short4& rect, short thickness);
        void DownWedge(const color32& color, const short4& rect, short thickness);

        FontGeometryInfo CalculateText(const short4& rect, const char* text, Font* font, const FontStyle& style);
        short4 Text(const color32& color, const FontGeometryInfo& info);
        short4 Text(const color32& color, const short4& rect, const char* text, Font* font, const FontStyle& style);

    private:
        IGUIAllocator* m_allocator = nullptr;
        short4 m_clipStack[CLIP_STACK_MAX + 1u]{};
        uint32_t m_clipIndices[CLIP_STACK_MAX + 1u]{};
        uint32_t m_clipIndex = 0u;
        uint32_t m_clipHead = 0u;
        uint16_t m_layer = 0u;
    };
}
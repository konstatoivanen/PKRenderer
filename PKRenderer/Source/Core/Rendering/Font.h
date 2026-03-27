#pragma once
#include "Core/Utilities/FastBuffer.h"
#include "Core/Math/Math.h"
#include "Core/Assets/Asset.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct FontRect
    {
        uint16_t character;
        uint16_t lineIndex;
        short4 rect;
        ushort4 texrect;
    };

    struct FontStyle
    {
        float2 align = PK_FLOAT2_ZERO;
        float2 spacing = PK_FLOAT2_ONE;
        float size = 1.0f;
        bool wrap = false;
        bool clip = false;
        FontStyle& SetAlign(const float2& _align) { align = _align; return *this; }
        FontStyle& SetSpacing(const float2& _spacing) { spacing = _spacing; return *this; }
        FontStyle& SetSize(float _size) { size = _size; return *this; }
        FontStyle& SetWrap(bool _wrap) { wrap = _wrap; return *this; }
        FontStyle& SetClip(bool _clip) { clip = _clip; return *this; }
    };

    struct Font : public Asset
    {
        struct Glyph
        {
            float advance = 0.0f;
            float4 rect = PK_FLOAT4_ZERO;
            ushort4 texrect = PK_USHORT4_ZERO;
            bool isWhiteSpace = true;
        };

        Font(const char* filepath);

        constexpr const Glyph& GetGlyph(const char asciichar) const { return m_glyphs[(uint8_t)asciichar]; }
        constexpr float GetLineHeight() const { return m_lineHeight; }
        constexpr float GetAscender() const { return m_ascender; }
        constexpr float GetDescender() const { return m_descender; }
        constexpr float GetUnderline() const { return m_underline; }
        constexpr float GetUnderlineThickness() const { return m_underlineThickness; }
        constexpr float GetAlignTop() const { return m_alignTop; }
        constexpr float GetAlignBottom() const { return m_alignBottom; }

        RHITexture* GetRHI();
        const RHITexture* GetRHI() const;

        static size_t CalculateMaxRectCount(const char* text, const Font* font);
        static size_t CalculateRects(const char* text, const Font* font, const short4& area_rect, const short4& clip_rect, const FontStyle& style, FontRect* out_rects, size_t max_rects);

    private:
        RHITextureRef m_texture = nullptr;
        Glyph m_glyphs[256]{};
        float m_lineHeight = 0.0f;
        float m_ascender = 0.0f;
        float m_descender = 0.0f;
        float m_underline = 0.0f;
        float m_underlineThickness = 0.0f;
        float m_alignTop = 0.0f;
        float m_alignBottom = 0.0f;
    };
}

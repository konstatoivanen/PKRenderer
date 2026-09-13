#pragma once
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

    struct FontGeometryInfo
    {
        FontStyle style{};
        Font* font = nullptr;
        const char* text = nullptr;
        short4 area_rect = PK_SHORT4_ZERO;
        short4 text_rect = PK_SHORT4_ZERO;
        short4 clip_rect = PK_SHORT4_ZERO;
        uint32_t rect_count = 0u;
        uint32_t line_count = 0u;
        uint32_t text_length = 0u;
    };

    struct Font : public Asset
    {
        typedef void (*OnVisibleRect)(void*, const FontRect&, uint32_t);

        struct Glyph
        {
            float advance = 0.0f;
            float4 rect = PK_FLOAT4_ZERO;
            ushort4 texrect = PK_USHORT4_ZERO;
            bool isWhiteSpace = true;
        };

        Font(const char* filepath);

        constexpr const Glyph& GetGlyph(const char ansichar) const { return m_glyphs[(uint8_t)ansichar]; }
        constexpr float GetLineHeight() const { return m_lineHeight; }
        constexpr float GetAscender() const { return m_ascender; }
        constexpr float GetDescender() const { return m_descender; }
        constexpr float GetUnderline() const { return m_underline; }
        constexpr float GetUnderlineThickness() const { return m_underlineThickness; }
        constexpr float GetAlignTop() const { return m_alignTop; }
        constexpr float GetAlignBottom() const { return m_alignBottom; }
        
        uint32_t GetAdvance(const char ansichar, const FontStyle& style) const;
        uint32_t GetLineHeight(const FontStyle& style) const;
        float GetLineAlignment(const FontStyle& style) const;
        float2 GetTexelSize() const;
        uint2 GetAtlasSize() const;
        RHITexture* GetRHI();
        const RHITexture* GetRHI() const;

        static uint32_t GenerateRects(const FontGeometryInfo& info, void* userData, OnVisibleRect onVisibleRect);
        static FontGeometryInfo GenerateGeometryInfo(Font* font, const FontStyle& style, const char* text, const short4& area_rect, const short4& clip_rect);

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

    template<>
    struct AssetTraits<Font>
    {
        constexpr static const char* Extension = "*.pkfont";
    };
}

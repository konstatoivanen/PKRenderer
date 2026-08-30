#include "PrecompiledHeader.h"
#include <PKAssets/PKAsset.h>
#include <PKAssets/PKAssetLoader.h>
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Math/Extended.h"
#include "Font.h"

namespace PK
{
    #define PK_FONT_FLIP_Y 1

    Font::Font(const char* filepath)
    {
        PKAssets::PKAsset asset{};
        PK_FATAL_ASSERT(PKAssets::OpenAsset(filepath, &asset) == 0, "Failed to open asset at path: %s", filepath);
        PK_FATAL_ASSERT(asset.header->type == PKAssets::PKAssetType::Font, "Trying to read a font from a non font file!")

        auto font = PKAssets::ReadAsFont(&asset);
        auto base = asset.rawData;
        auto pCharacters = font->characters.Get(base);

        m_lineHeight = font->lineHeight;
        m_ascender = font->ascender;
        m_descender = font->descender;
        m_underline = font->underline;
        m_underlineThickness = font->underlineThickness;

        auto miny = 0.0f;
        auto maxy = 0.0f;

        for (auto i = 0u; i < font->characterCount; ++i)
        {
            auto& assetCharacter = pCharacters[i];
            auto& glyph = m_glyphs[assetCharacter.unicode];
            glyph.advance = assetCharacter.advance;
            glyph.rect.x = assetCharacter.rect[0];
            glyph.rect.y = assetCharacter.rect[1];
            glyph.rect.z = assetCharacter.rect[2];
            glyph.rect.w = assetCharacter.rect[3];
            glyph.texrect.x = assetCharacter.texrect[0];
            glyph.texrect.y = assetCharacter.texrect[1];
            glyph.texrect.z = assetCharacter.texrect[2];
            glyph.texrect.w = assetCharacter.texrect[3];
            glyph.isWhiteSpace = assetCharacter.isWhiteSpace;

            #if PK_FONT_FLIP_Y
            glyph.rect.y *= -1.0f;
            glyph.rect.w *= -1.0f;
            #endif

            miny = math::min(miny, glyph.rect.y + glyph.rect.w);
            miny = math::min(miny, glyph.rect.y);
            maxy = math::max(maxy, glyph.rect.y + glyph.rect.w);
            maxy = math::max(maxy, glyph.rect.y);
        }

        m_alignTop = -miny;
        m_alignBottom = m_lineHeight - maxy;

        TextureDescriptor descriptor{};
        descriptor.usage = TextureUsage::DefaultDisk;
        descriptor.resolution = { font->atlasResolution[0], font->atlasResolution[1], 1 };
        descriptor.layers = 1u;
        descriptor.levels = 1u;
        descriptor.format = PKAssets::PK_FONT_FORMAT;
        descriptor.type = TextureType::Texture2D;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;

        m_texture = RHI::CreateTexture(descriptor, String::ToFilePathStem<64>(filepath));

        TextureDataRegion dataRegion;
        dataRegion.bufferOffset = 0u;
        dataRegion.level = 0u;
        dataRegion.layer = 0u;
        dataRegion.layers = 1u;
        dataRegion.offset = PK_UINT3_ZERO;
        dataRegion.extent = descriptor.resolution;;

        RHI::GetCommandBuffer(QueueType::Transfer)->CopyToTexture(m_texture.get(),
            font->atlasData.Get(base),
            font->atlasDataSize,
            &dataRegion,
            1u);

        PKAssets::CloseAsset(&asset);
    }

    uint32_t Font::GetAdvance(const char ansichar, const FontStyle& style) const { return (uint32_t)math::round(GetGlyph(ansichar).advance * style.spacing.x * style.size); }
    uint32_t Font::GetLineHeight(const FontStyle& style) const { return (uint32_t)math::round(m_lineHeight * style.spacing.y * style.size); }
    float Font::GetLineAlignment(const FontStyle& style) const { return math::lerp(GetAlignTop(), GetAlignBottom(), style.align.y) * style.size; }
    float2 Font::GetTexelSize() const { return m_texture->GetTexelSize().xy(); }
    uint2 Font::GetAtlasSize() const { return m_texture->GetResolution().xy(); }
    RHITexture* Font::GetRHI() { return m_texture.get(); }
    const RHITexture* Font::GetRHI() const { return m_texture.get(); }

    uint32_t Font::GenerateRects(const FontGeometryInfo& info, void* userData, OnVisibleRect onVisibleRect)
    {
        const auto font = info.font;
        const auto text = info.text;
        const auto& style = info.style;
        const auto max_width = info.area_rect.z;
        const auto max_height = info.area_rect.w;
        const auto line_height = (int16_t)font->GetLineHeight(style);
        const auto line_align = font->GetLineAlignment(style);

        auto offset_y = (int16_t)math::round((max_height - info.line_count * line_height) * style.align.y + line_align);
        auto offsets_x = PK_STACK_ALLOC(int16_t, info.line_count);
        auto rect_count = 0u;

        for (auto i = 0u, line_y = 0u, line_x = 0u; i <= info.text_length; ++i)
        {
            if (i == info.text_length)
            {
                offsets_x[line_y] = (int16_t)((max_width - line_x) * style.align.x);
                break;
            }

            const auto advance = font->GetAdvance(text[i], style);

            if (text[i] == '\n' || (style.wrap && (int16_t)(line_x + advance) > max_width))
            {
                offsets_x[line_y++] = int16_t((max_width - line_x) * style.align.x);
                line_x = 0u;
            }

            line_x += advance;
        }

        for (auto i = 0u, line_y = 0u, line_x = 0u; i < info.text_length; ++i)
        {
            const auto& glyph = font->GetGlyph(text[i]);
            const auto advance = font->GetAdvance(text[i], style);

            if (text[i] == '\n' || (style.wrap && (int16_t)(line_x + advance) > max_width))
            {
                line_y++;
                line_x = 0u;
            }

            if (!glyph.isWhiteSpace)
            {
                FontRect rect{};
                rect.character = text[i];
                rect.lineIndex = (uint16_t)line_y;
                rect.rect.x = (int16_t)line_x + offsets_x[line_y] + (int16_t)math::round(glyph.rect.x * style.size);
                rect.rect.y = (int16_t)line_y * line_height + offset_y + (int16_t)math::round(glyph.rect.y * style.size);
                rect.rect.z = (int16_t)math::round(glyph.rect.z * style.size);
                rect.rect.w = (int16_t)math::round(glyph.rect.w * style.size);
                rect.rect.x += info.area_rect.x;
                rect.rect.y += info.area_rect.y;
                rect.texrect = glyph.texrect;

                if (!style.clip || math::intersectRects(rect.rect, info.clip_rect))
                {
                    onVisibleRect(userData, rect, rect_count);
                    rect_count++;
                }
            }

            line_x += advance;
        }

        return rect_count;
    }

    FontGeometryInfo Font::GenerateGeometryInfo(Font* font, const FontStyle& style, const char* text, const short4& area_rect, const short4& clip_rect)
    {
        FontGeometryInfo info;
        info.style = style;
        info.font = font;
        info.text = text;
        info.text_rect = short4(+PK_SHORT2_MAX, -PK_SHORT2_MAX);
        info.area_rect = area_rect;
        info.rect_count = 0u;
        info.clip_rect = clip_rect;
        info.line_count = 1u;
        info.text_length = strlen(text);

        if (info.text_length == 0ull)
        {
            return info;
        }

        for (auto i = 0u, line_x = 0u; i < info.text_length; ++i)
        {
            const auto advance = font->GetAdvance(text[i], style);

            if (text[i] == '\n' || (style.wrap && (int16_t)(line_x + advance) > area_rect.z))
            {
                info.line_count++;
                line_x = 0u;
            }

            line_x += advance;
        }

        info.rect_count = GenerateRects(info, &info, [](void* userData, const FontRect& rect, [[maybe_unused]] uint32_t index)
        {
            auto* info = static_cast<FontGeometryInfo*>(userData);
            const auto sminmax = short4(rect.rect.x, rect.rect.y, rect.rect.x + rect.rect.z, rect.rect.y + rect.rect.w);
            info->text_rect.x = math::min(info->text_rect.x, sminmax.x, sminmax.z);
            info->text_rect.y = math::min(info->text_rect.y, sminmax.y, sminmax.w);
            info->text_rect.z = math::max(info->text_rect.z, sminmax.x, sminmax.z);
            info->text_rect.w = math::max(info->text_rect.w, sminmax.y, sminmax.w);
        });

        info.text_rect = short4(info.text_rect.xy(), info.text_rect.zw() - info.text_rect.xy());
        
        return info;
    }
}

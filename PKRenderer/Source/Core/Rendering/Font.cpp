#include "PrecompiledHeader.h"
#include <PKAssets/PKAsset.h>
#include <PKAssets/PKAssetLoader.h>
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "Font.h"

namespace PK
{
    #define PK_FONT_FLIP_Y 1

    Font::Font(const char* filepath)
    {
        PKAssets::PKAsset asset;

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

            miny = glm::min(miny, glyph.rect.y + glyph.rect.w);
            miny = glm::min(miny, glyph.rect.y);
            maxy = glm::max(maxy, glyph.rect.y + glyph.rect.w);
            maxy = glm::max(maxy, glyph.rect.y);
        }

        m_alignTop = -miny;
        m_alignBottom = m_lineHeight - maxy;

        TextureDescriptor descriptor{};
        descriptor.usage = TextureUsage::DefaultDisk;
        descriptor.resolution = { font->atlasResolution[0], font->atlasResolution[1], 1 };
        descriptor.layers = 1u;
        descriptor.levels = 1u;
        descriptor.format = TextureFormat::RGBA8;
        descriptor.type = TextureType::Texture2D;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;

        m_texture = RHI::CreateTexture(descriptor, Parse::GetFilePathStem(filepath));

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

    RHITexture* Font::GetRHI() { return m_texture.get(); }
    
    const RHITexture* Font::GetRHI() const { return m_texture.get(); }
    
    size_t Font::CalculateMaxRectCount(const char* text, const Font* font)
    {
        const auto length = strlen(text);

        if (length == 0)
        {
            return 0;
        }

        auto rect_count = 0u;

        // Count lines 
        for (auto i = 0u; i < length; ++i)
        {
            if (!font->GetGlyph(text[i]).isWhiteSpace)
            {
                rect_count++;
            }
        }

        return rect_count;
    }

    size_t Font::CalculateRects(const char* text, const Font* font, const short4& area_rect, const short4& clip_rect, const FontStyle& style, FontRect* out_rects, size_t max_rects)
    {
        const auto length = strlen(text);

        if (length == 0)
        {
            return 0ull;
        }

        const auto char_h = (int32_t)glm::round(font->GetLineHeight() * style.spacing.y * style.size);
        const auto char_w = style.spacing.x * style.size;
        auto line_x = 0;
        auto line_y = 0;

        // Count lines 
        for (auto i = 0u; i <= length; ++i)
        {
            if (i == length)
            {
                line_y++;
                break;
            }

            auto x = (int32_t)glm::round(font->GetGlyph(text[i]).advance * char_w);

            if (text[i] == '\n' || (style.wrap && line_x + x > area_rect.z))
            {
                line_y++;
                line_x = 0;
            }

            line_x += x;
        }

        auto offsets_x = PK_STACK_ALLOC(int16_t, line_y);
        line_y = 0;
        line_x = 0;

        // Build line widths
        for (auto i = 0u; i <= length; ++i)
        {
            if (i == length)
            {
                offsets_x[line_y++] = int16_t((area_rect.z - line_x) * style.align.x);
                break;
            }

            auto x = (int32_t)glm::round(font->GetGlyph(text[i]).advance * char_w);

            if (text[i] == '\n' || (style.wrap && line_x + x > area_rect.z))
            {
                offsets_x[line_y++] = int16_t((area_rect.z - line_x) * style.align.x);
                line_x = 0;
            }

            line_x += x;
        }

        auto clip_min = glm::min(short2(clip_rect.xy), short2(clip_rect.xy + clip_rect.zw));
        auto clip_max = glm::max(short2(clip_rect.xy), short2(clip_rect.xy + clip_rect.zw));

        // Top/Bottom alignment needs to take into account min/max rect boundaries as we otherwise add padding.
        auto line_align = glm::mix(font->GetAlignTop(), font->GetAlignBottom(), style.align.y) * style.size;
        auto offset_y = (int16_t)glm::round((area_rect.w - line_y * char_h) * style.align.y + line_align);
        auto rect_count = 0u;
        line_x = 0;
        line_y = 0;

        // Output characters.
        for (auto i = 0u; i < length; ++i)
        {
            auto& c = font->GetGlyph(text[i]);
            auto x = (uint32_t)glm::round(c.advance * char_w);

            if (text[i] == '\n' || (style.wrap && line_x + x > (uint32_t)area_rect.z))
            {
                line_y++;
                line_x = 0;
            }

            if (!c.isWhiteSpace && rect_count < max_rects)
            {
                FontRect rect{};
                rect.character = text[i];
                rect.lineIndex = line_y;
                rect.rect.x = line_x + offsets_x[line_y] + area_rect.x + (int16_t)glm::round(c.rect.x * style.size);
                rect.rect.y = line_y * char_h + offset_y + area_rect.y + (int16_t)glm::round(c.rect.y * style.size);
                rect.rect.z = (int16_t)glm::round(c.rect.z * style.size);
                rect.rect.w = (int16_t)glm::round(c.rect.w * style.size);
                rect.texrect = c.texrect;

                auto is_visible = true;

                if (style.clip)
                {
                    auto rmin = glm::min(short2(rect.rect.xy), short2(rect.rect.xy + rect.rect.zw));
                    auto rmax = glm::max(short2(rect.rect.xy), short2(rect.rect.xy + rect.rect.zw));
                    is_visible &= rmin.x < clip_max.x&& rmax.y < clip_max.y&& rmax.x > clip_min.x&& rmax.y > clip_min.y;
                }

                if (is_visible)
                {
                    out_rects[rect_count++] = rect;
                }
            }

            line_x += x;
        }

        return rect_count;
    }
}

template<>
bool PK::Asset::IsValidExtension<PK::Font>(const wchar_t* extension) { return wcscmp(extension, L".pkfont") == 0; }

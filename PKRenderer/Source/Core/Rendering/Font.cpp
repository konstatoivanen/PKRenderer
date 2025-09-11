#include "PrecompiledHeader.h"
#include <filesystem>
#include <PKAssets/PKAsset.h>
#include <PKAssets/PKAssetLoader.h>
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "Font.h"

namespace PK
{
    RHITexture* Font::GetRHI() { return m_texture.get(); }
    
    const RHITexture* Font::GetRHI() const { return m_texture.get(); }
    
    void Font::AssetImport(const char* filepath)
    {
        PKAssets::PKAsset asset;

        PK_THROW_ASSERT(PKAssets::OpenAsset(filepath, &asset) == 0, "Failed to open asset at path: %s", filepath);
        PK_THROW_ASSERT(asset.header->type == PKAssets::PKAssetType::Font, "Trying to read a font from a non font file!")

        auto font = PKAssets::ReadAsFont(&asset);
        auto base = asset.rawData;
        auto pCharacters = font->characters.Get(base);

        for (auto i = 0u; i < font->characterCount; ++i)
        {
            auto& assetCharacter = pCharacters[i];
            auto& character = m_characters[assetCharacter.unicode];
            character.advance = assetCharacter.advance;
            character.rect.x = assetCharacter.rect[0];
            character.rect.y = assetCharacter.rect[1];
            character.rect.z = assetCharacter.rect[2];
            character.rect.w = assetCharacter.rect[3];
            character.texrect.x = assetCharacter.texrect[0];
            character.texrect.y = assetCharacter.texrect[1];
            character.texrect.z = assetCharacter.texrect[2];
            character.texrect.w = assetCharacter.texrect[3];
            character.isWhiteSpace = assetCharacter.isWhiteSpace;
        }

        TextureDescriptor descriptor{};
        descriptor.usage = TextureUsage::DefaultDisk;
        descriptor.resolution = { font->atlasResolution[0], font->atlasResolution[1], 1 };
        descriptor.layers = 1u;
        descriptor.levels = 1u;
        descriptor.format = TextureFormat::RGBA8;
        descriptor.type = TextureType::Texture2D;
        descriptor.sampler.filterMin = FilterMode::Bilinear;
        descriptor.sampler.filterMag = FilterMode::Bilinear;

        m_texture = RHI::CreateTexture(descriptor, std::filesystem::path(GetFileName()).stem().string().c_str());

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
    
    void TextGeometryBuilder::Initialize(const char* text, Font* font, TextAlign alignx, TextAlign aligny, float size, float lineSpacing)
    {
        m_font = font;
        m_text = text;
        m_size = size;
        m_geometryCount = 0u;
        m_currentAdvance = 0u;
        m_currentLineIndex = 0u;
        m_currentIndex = 0;
        m_textLength = strlen(text);
        
        auto pxLineSpacing = (uint32_t)glm::round(lineSpacing * m_size);
        auto lineCount = 0u;
        auto lineLength = 0u;
        auto totalHeight = 0;

        for (auto i = 0u; i < m_textLength; ++i)
        {
            auto ch = text[i];
            auto& character = font->GetCharacter(ch);
            auto advance = (uint32_t)glm::round(character.advance * m_size);

            if (ch == '\n')
            {
                m_lineBounds.resize(glm::max((uint32_t)m_lineBounds.size(), lineCount + 1u));
                m_lineBounds[lineCount++] = { lineLength, -totalHeight, i };
                totalHeight += pxLineSpacing;
                lineLength = 0u;
            }
            else
            {
                m_geometryCount += character.isWhiteSpace ? 0u : 1u;
                lineLength += advance;
            }
        }

        m_lineBounds.resize(glm::max((uint32_t)m_lineBounds.size(), lineCount + 1u));
        m_lineBounds[lineCount++] = { lineLength, -totalHeight, m_textLength };
        totalHeight += pxLineSpacing;

        for (auto i = 0u; i < lineCount; ++i)
        {
            switch (alignx)
            {
                case TextAlign::Start: m_lineBounds[i].x = 0; break;
                case TextAlign::End: m_lineBounds[i].x = -m_lineBounds[i].x; break;
                case TextAlign::Center: m_lineBounds[i].x = -m_lineBounds[i].x / 2; break;
            }

            switch (aligny)
            {
                case TextAlign::Start: m_lineBounds[i].y = m_lineBounds[i].y - pxLineSpacing; break;
                case TextAlign::End: m_lineBounds[i].y = m_lineBounds[i].y + totalHeight; break;
                case TextAlign::Center: m_lineBounds[i].y = m_lineBounds[i].y + (totalHeight / 2); break;
            }
        }
    }

    TextGeometryBuilder::Geometry* TextGeometryBuilder::GetNextGeometry()
    {
        if (m_currentIndex >= m_textLength)
        {
            return nullptr;
        }

        auto ch = m_text[m_currentIndex];
        auto lineBounds = &m_lineBounds.at(m_currentLineIndex);

        while (lineBounds->z == (int)m_currentIndex)
        {
            lineBounds = &m_lineBounds.at(++m_currentLineIndex);
            ch = m_text[++m_currentIndex];
            m_currentAdvance = 0.0f;
        }

        m_currentIndex++;
        auto& character = m_font->GetCharacter(ch);
        m_geometry.isWhiteSpace = character.isWhiteSpace;
        m_geometry.texrect = character.texrect;
        m_geometry.rect.x = lineBounds->x + glm::round(m_currentAdvance + character.rect.x * m_size);
        m_geometry.rect.y = lineBounds->y + glm::round(character.rect.y * m_size);
        m_geometry.rect.z = glm::round(character.rect.z * m_size);
        m_geometry.rect.w = glm::round(character.rect.w * m_size);
        m_geometry.color = PK_COLOR32_WHITE;
        m_currentAdvance += character.advance * m_size;
        return &m_geometry;
    }
}

template<>
bool PK::Asset::IsValidExtension<PK::Font>(const char* extension) { return strcmp(extension, ".pkfont") == 0; }

template<>
PK::Ref<PK::Font> PK::Asset::Create() { return PK::CreateRef<Font>(); }

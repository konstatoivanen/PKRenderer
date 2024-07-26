#pragma once
#include <vector>
#include "Core/Math/Math.h"
#include "Core/Assets/Asset.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct Font : public AssetWithImport<>
    {
        struct Character
        {
            float advance = 0.0f;
            float4 rect = PK_FLOAT4_ZERO;
            ushort4 texrect = PK_USHORT4_ZERO;
            bool isWhiteSpace = true;
        };

        Font() {};

        constexpr const Character& GetCharacter(const char character) const { return m_characters[(uint8_t)character]; }

        RHITexture* GetRHI();
        const RHITexture* GetRHI() const;

        void AssetImport(const char* filepath) final;

    private:
        RHITextureRef m_texture = nullptr;
        Character m_characters[255]{};
    };

    enum class TextAlign
    {
        Start,
        End,
        Center,
    };

    struct TextGeometryBuilder
    {
        struct Geometry
        {
            short4 rect;
            ushort4 texrect;
            color32 color;
            bool isWhiteSpace;
        };

        void Initialize(const char* text, Font* font, TextAlign alignx, TextAlign aligny, float size, float lineSpacing);
        Geometry* GetNextGeometry();
        constexpr uint32_t GetVisibleGeometryCount() const { return m_geometryCount; }

    private:
        Font* m_font = nullptr;
        const char* m_text = nullptr;
        float m_size = 0.0f;
        uint32_t m_geometryCount = 0u;
        std::vector<int3> m_lineBounds;

        float m_currentAdvance = 0.0f;
        uint32_t m_currentLineIndex = 0;
        uint32_t m_currentIndex = 0;
        uint32_t m_textLength = 0;
        Geometry m_geometry{};
    };
}
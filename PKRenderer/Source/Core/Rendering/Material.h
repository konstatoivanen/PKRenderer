#pragma once
#include "Core/Utilities/FastBuffer.h"
#include "Core/Assets/Asset.h"
#include "Core/Rendering/ShaderPropertyBlock.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct Material : public Asset, public ShaderPropertyWriter
    {
        constexpr static const uint32_t INLINE_SIZE = 32ull;

        Material() {}

        Material(ShaderAsset* shader) : 
            m_shader(shader)
        { 
            ReservePropertyBuffer();
        }

        Material(ShaderAsset* shader, ShaderAsset* shadowShader) :
            m_shader(shader),
            m_shaderShadow(shadowShader)
        { 
            ReservePropertyBuffer();
        }

        Material(const char* filepath);

        constexpr ShaderAsset* GetShader() const { return m_shader; }
        constexpr ShaderAsset* GetShaderShadow() const { return m_shaderShadow; }

        size_t GetPropertyStride() const;
        bool SupportsKeyword(const NameID keyword) const;
        void CopyTo(char* dst, RHITextureBindSet* textureSet) const;

    private:
        void ReservePropertyBuffer();

        ShaderAsset* m_shader = nullptr;
        ShaderAsset* m_shaderShadow = nullptr;
        FastBuffer<uint8_t, INLINE_SIZE> m_propertyBuffer;
    };

    struct MaterialTarget
    {
        Ref<Material> material;
        uint32_t submesh;
    };
}

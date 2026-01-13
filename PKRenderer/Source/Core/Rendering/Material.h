#pragma once
#include "Core/Assets/Asset.h"
#include "Core/Rendering/BindSet.h"
#include "Core/Rendering/ShaderPropertyBlock.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct Material : public Asset, public ShaderPropertyBlock
    {
        Material() : 
            ShaderPropertyBlock(0u, 0u) 
        {
        }

        Material(ShaderAsset* shader) :
            ShaderPropertyBlock(0u, 0u), 
            m_shader(shader)
        { 
            InitializeShaderLayout(); 
        }

        Material(ShaderAsset* shader, ShaderAsset* shadowShader) :
            ShaderPropertyBlock(0u, 0u), 
            m_shader(shader),
            m_shaderShadow(shadowShader)
        { 
            InitializeShaderLayout(); 
        }

        Material(const char* filepath);

        constexpr ShaderAsset* GetShader() const { return m_shader; }
        constexpr ShaderAsset* GetShaderShadow() const { return m_shaderShadow; }

        size_t GetPropertyStride() const;
        bool SupportsKeyword(const NameID keyword) const;
        bool SupportsKeywords(const NameID* keywords, const uint32_t count) const;

        void CopyTo(char* dst, BindSet<RHITexture>* textureSet) const;

    private:
        void InitializeShaderLayout(uint32_t minSize = 0u, uint32_t minPropertyCount = 0u);

        ShaderAsset* m_shader = nullptr;
        ShaderAsset* m_shaderShadow = nullptr;
    };

    struct MaterialTarget
    {
        Ref<Material> material;
        uint32_t submesh;
    };
}

#pragma once
#include "Core/Assets/Asset.h"
#include "Core/Rendering/BindSet.h"
#include "Core/Rendering/ShaderPropertyBlock.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct Material : public AssetWithImport<>, public ShaderPropertyBlock
    {
        Material() : ShaderPropertyBlock(1024ull, 16ull) 
        {
        }

        Material(ShaderAsset* shader) :
            ShaderPropertyBlock(1024, 16ull), 
            m_shader(shader) 
        { 
            InitializeShaderLayout(); 
        }

        Material(ShaderAsset* shader, ShaderAsset* shadowShader) :
            ShaderPropertyBlock(1024ull, 16ull), 
            m_shader(shader), 
            m_shadowShader(shadowShader) 
        { 
            InitializeShaderLayout(); 
        }

        constexpr ShaderAsset* GetShader() const { return m_shader; }
        constexpr ShaderAsset* GetShadowShader() const { return m_shadowShader; }

        bool SupportsKeyword(const NameID keyword) const;
        bool SupportsKeywords(const NameID* keywords, const uint32_t count) const;

        void CopyTo(char* dst, BindSet<RHITexture>* textureSet) const;

        void AssetImport(const char* filepath) final;

        void InitializeShaderLayout();

    private:
        ShaderAsset* m_shader = nullptr;
        ShaderAsset* m_shadowShader = nullptr;
    };

    struct MaterialTarget
    {
        Material* material;
        uint32_t submesh;
    };
}
#pragma once
#include "Core/Assets/Asset.h"
#include "Graphics/BindSet.h"
#include "Graphics/ShaderPropertyBlock.h"
#include "Graphics/GraphicsFwd.h"

namespace PK::Graphics
{
    struct Material : public Core::Assets::AssetWithImport<>, public ShaderPropertyBlock
    {
        Material() : ShaderPropertyBlock(1024ull, 16ull) 
        {
        }

        Material(Shader* shader) : 
            ShaderPropertyBlock(1024, 16ull), 
            m_shader(shader) 
        { 
            InitializeShaderLayout(); 
        }

        Material(Shader* shader, Shader* shadowShader) : 
            ShaderPropertyBlock(1024ull, 16ull), 
            m_shader(shader), 
            m_shadowShader(shadowShader) 
        { 
            InitializeShaderLayout(); 
        }

        constexpr Shader* GetShader() const { return m_shader; }
        constexpr Shader* GetShadowShader() const { return m_shadowShader; }

        bool SupportsKeyword(const Utilities::NameID keyword) const;
        bool SupportsKeywords(const Utilities::NameID* keywords, const uint32_t count) const;

        void CopyTo(char* dst, BindSet<Texture>* textureSet) const;

        void AssetImport(const char* filepath) final;

        void InitializeShaderLayout();

    private:
        Shader* m_shader = nullptr;
        Shader* m_shadowShader = nullptr;
    };

    struct MaterialTarget
    {
        Material* material;
        uint32_t submesh;
    };
}
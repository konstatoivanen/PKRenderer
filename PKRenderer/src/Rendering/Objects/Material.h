#pragma once
#include "Core/Services/AssetDataBase.h"
#include "Rendering/Objects/BindSet.h"
#include "Rendering/Objects/ShaderPropertyBlock.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::Rendering::Objects
{
    class Material : public Core::Services::Asset, public Core::Services::IAssetImportSimple, public ShaderPropertyBlock
    {
        friend Utilities::Ref<Material> Core::Services::AssetImporters::Create();

        public:
            Material() : ShaderPropertyBlock(1024) {}
            Material(RHI::Objects::Shader* shader) : ShaderPropertyBlock(1024), m_shader(shader) { InitializeShaderLayout(); }
            Material(RHI::Objects::Shader* shader, RHI::Objects::Shader* shadowShader) : ShaderPropertyBlock(1024), m_shader(shader), m_shadowShader(shadowShader) { InitializeShaderLayout(); }
            constexpr RHI::Objects::Shader* GetShader() const { return m_shader; }
            constexpr RHI::Objects::Shader* GetShadowShader() const { return m_shadowShader; }
            inline bool SupportsKeyword(const uint32_t hashId) const { return m_shader->SupportsKeyword(hashId); }
            inline bool SupportsKeywords(const uint32_t* hashIds, const uint32_t count) const { return m_shader->SupportsKeywords(hashIds, count); }

            void CopyTo(char* dst, BindSet<RHI::Objects::Texture>* textureSet) const;

            void Import(const char* filepath) final;

            void InitializeShaderLayout();

        private:
            RHI::Objects::Shader* m_shader = nullptr;
            RHI::Objects::Shader* m_shadowShader = nullptr;
    };

    struct MaterialTarget
    {
        Material* material;
        uint32_t submesh;
    };
}
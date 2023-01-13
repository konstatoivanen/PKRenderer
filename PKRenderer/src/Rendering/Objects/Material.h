#pragma once
#include "Core/Services/AssetDataBase.h"
#include "Rendering/Objects/ShaderPropertyBlock.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/BindSet.h"

namespace PK::Rendering::Objects
{
    class Material : public Core::Services::Asset, public Core::Services::IAssetImportSimple, public ShaderPropertyBlock
    {
        // @TODO refactor this to be derived from a config or generate at runtime?.
        constexpr static const char* DEFAULT_PATH_TEXTURE_BLACK = "res/textures/default/T_Black.ktx2";
        constexpr static const char* DEFAULT_PATH_TEXTURE_WHITE = "res/textures/default/T_White.ktx2";

        friend Utilities::Ref<Material> Core::Services::AssetImporters::Create();

        public:
            Material() : ShaderPropertyBlock(1024) {}
            Material(Shader* shader) : ShaderPropertyBlock(1024), m_shader(shader) { InitializeShaderLayout(); }
            Material(Shader* shader, Shader* shadowShader) : ShaderPropertyBlock(1024), m_shader(shader), m_shadowShader(shadowShader) { InitializeShaderLayout(); }
            constexpr Shader* GetShader() const { return m_shader; }
            constexpr Shader* GetShadowShader() const { return m_shadowShader; }
            inline bool SupportsKeyword(const uint32_t hashId) const { return m_shader->SupportsKeyword(hashId); }
            inline bool SupportsKeywords(const uint32_t* hashIds, const uint32_t count) const { return m_shader->SupportsKeywords(hashIds, count); }

            void CopyTo(char* dst, BindSet<Texture>* textureSet) const;

            void Import(const char* filepath) override final;

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
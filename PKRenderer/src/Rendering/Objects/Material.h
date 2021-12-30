#pragma once
#include "Core/Services/AssetDataBase.h"
#include "Rendering/Objects/ShaderPropertyBlock.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/BindSet.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::Structs;

    class Material : public Asset, public ShaderPropertyBlock
    {
        friend Ref<Material> AssetImporters::Create();

        public:
            Material() : ShaderPropertyBlock(1024) {}
            Material(Shader* shader) : ShaderPropertyBlock(1024) { m_shader = shader; m_cachedShaderAssetId = shader->GetAssetID(); }
            constexpr Shader* GetShader() const { return m_shader; }
            constexpr AssetID GetShaderAssetID() const { return m_cachedShaderAssetId; }
            inline bool SupportsKeyword(const uint32_t hashId) const { return m_shader->SupportsKeyword(hashId); }
            inline bool SupportsKeywords(const uint32_t* hashIds, const uint32_t count) const { return m_shader->SupportsKeywords(hashIds, count); }

            void CopyTo(char* dst, BindSet<Texture>* textureSet) const;

            void Import(const char* filepath) override final;

        private:
            AssetID m_cachedShaderAssetId = 0;
            Shader* m_shader = nullptr;
    };
}
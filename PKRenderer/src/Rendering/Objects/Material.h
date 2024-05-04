#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/Assets/Asset.h"
#include "Rendering/Objects/BindSet.h"
#include "Rendering/Objects/ShaderPropertyBlock.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Shader)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::RHI::Objects, class Texture)

namespace PK::Rendering::Objects
{
    class Material : public Core::Assets::AssetWithImport<>, public ShaderPropertyBlock
    {
    public:
        Material() : ShaderPropertyBlock(1024) {}
        Material(RHI::Objects::Shader* shader) : ShaderPropertyBlock(1024), m_shader(shader) { InitializeShaderLayout(); }
        Material(RHI::Objects::Shader* shader, RHI::Objects::Shader* shadowShader) : ShaderPropertyBlock(1024), m_shader(shader), m_shadowShader(shadowShader) { InitializeShaderLayout(); }

        constexpr RHI::Objects::Shader* GetShader() const { return m_shader; }
        constexpr RHI::Objects::Shader* GetShadowShader() const { return m_shadowShader; }

        bool SupportsKeyword(const Utilities::NameID keyword) const;
        bool SupportsKeywords(const Utilities::NameID* keywords, const uint32_t count) const;

        void CopyTo(char* dst, BindSet<RHI::Objects::Texture>* textureSet) const;

        void AssetImport(const char* filepath) final;

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
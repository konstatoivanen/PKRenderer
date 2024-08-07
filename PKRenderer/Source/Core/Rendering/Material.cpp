#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Yaml/ConvertMathTypes.h"
#include "Core/Yaml/ConvertTextureAsset.h"
#include "Core/Yaml/ConvertShader.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/TextureAsset.h"
#include "Material.h"

namespace PK
{
    bool Material::SupportsKeyword(const NameID keyword) const { return m_shader->SupportsKeyword(keyword); }

    bool Material::SupportsKeywords(const NameID* keywords, const uint32_t count) const { return m_shader->SupportsKeywords(keywords, count); }

    void Material::CopyTo(char* dst, BindSet<RHITexture>* textureSet) const
    {
        auto& layout = m_shader->GetMaterialPropertyLayout();

        memcpy(dst, m_buffer, layout.GetAlignedStride());

        for (auto& element : layout)
        {
            switch (element.format)
            {
                case ElementType::Texture2DHandle:
                {
                    auto texIndex = textureSet->Set(*Get<RHITexture*>(element.name));
                    memcpy(dst + element.alignedOffset, &texIndex, sizeof(int32_t));
                }
                break;
                default: break;
            }
        }
    }

    void Material::AssetImport(const char* filepath)
    {
        YAML::Node root = YAML::LoadFile(filepath);

        auto data = root["Material"];
        auto shaderPathProp = data["Shader"];
        auto shadowShaderPathProp = data["ShadowShader"];
        auto keywords = data["Keywords"];
        auto properties = data["Properties"];

        PK_THROW_ASSERT(data, "Could not locate material (%s) header in file.", filepath);
        PK_THROW_ASSERT(shaderPathProp, "Material (%s) doesn't define a shader.", filepath);

        m_shader = shaderPathProp.as<ShaderAsset*>();
        InitializeShaderLayout();

        if (shadowShaderPathProp)
        {
            m_shadowShader = shadowShaderPathProp.as<ShaderAsset*>();
        }

        if (keywords)
        {
            for (auto keyword : keywords)
            {
                Set<bool>(NameID(keyword.as<std::string>()), true);
            }
        }

        if (properties)
        {
            for (auto property : properties)
            {
                auto propertyName = property.first.as<std::string>();
                auto type = property.second["Type"];

                if (!type)
                {
                    continue;
                }

                auto nameId = NameID(propertyName);
                auto typeName = type.as<std::string>();
                auto elementType = PKAssets::GetElementType(typeName.c_str());
                auto values = property.second["Value"];

                switch (elementType)
                {
                    case ElementType::Float: Set(nameId, values.as<float>()); break;
                    case ElementType::Float2: Set(nameId, values.as<float2>()); break;
                    case ElementType::Float3: Set(nameId, values.as<float3>()); break;
                    case ElementType::Float4: Set(nameId, values.as<float4>()); break;
                    case ElementType::Float2x2: Set(nameId, values.as<float2x2>()); break;
                    case ElementType::Float3x3: Set(nameId, values.as<float3x3>()); break;
                    case ElementType::Float4x4: Set(nameId, values.as<float4x4>()); break;
                    case ElementType::Float3x4: Set(nameId, values.as<float3x4>()); break;
                    case ElementType::Int: Set(nameId, values.as<int>()); break;
                    case ElementType::Int2: Set(nameId, values.as<int2>()); break;
                    case ElementType::Int3: Set(nameId, values.as<int3>()); break;
                    case ElementType::Int4: Set(nameId, values.as<int4>()); break;
                    case ElementType::Texture2DHandle: Set(nameId, values.as<TextureAsset*>()->GetRHI()); break;
                    case ElementType::Texture3DHandle: Set(nameId, values.as<TextureAsset*>()->GetRHI()); break;
                    case ElementType::TextureCubeHandle: Set(nameId, values.as<TextureAsset*>()->GetRHI()); break;
                    default: PK_LOG_WARNING("Unsupported material parameter type"); break;
                }
            }
        }
    }

    void Material::InitializeShaderLayout()
    {
        Clear();

        auto builtIns = RHI::GetBuiltInResources();

        PK_THROW_ASSERT(m_shader->SupportsMaterials(), "Shader is doesn't support materials!");
        ReserveLayout(m_shader->GetMaterialPropertyLayout());

        for (auto& element : m_shader->GetMaterialPropertyLayout())
        {
            switch (element.format)
            {
                case ElementType::Texture2DHandle: Set(element.name, builtIns->BlackTexture2D.get()); break;
                default: break;
            }
        }
    }
}

template<>
bool PK::Asset::IsValidExtension<PK::Material>(const std::string& extension) { return extension.compare(".material") == 0; }

template<>
PK::Ref<PK::Material> PK::Asset::Create() { return PK::CreateRef<Material>(); }
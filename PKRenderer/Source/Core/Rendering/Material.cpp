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
    static void CalculateMaterialSize(const YAML::Node& properties, const YAML::Node& keywords, uint32_t* outSize, uint32_t* outCount)
    {
        *outSize = 0u;
        *outCount = 0u;

        if (keywords)
        {
            *outSize += sizeof(bool) * keywords.size();
            *outCount += (uint32_t)keywords.size();
        }

        for (auto property : properties)
        {
            auto type = property.second["Type"];

            if (type)
            {
                auto elementType = PKAssets::GetElementType(type.as<std::string>().c_str());

                *outSize += PKAssets::GetElementSize(elementType);
                (*outCount)++;

                if (PKAssets::GetElementIsResourceHandle(elementType))
                {
                    *outSize += (uint32_t)sizeof(RHITexture*);
                    (*outCount)++;
                }
            }
        }
    }

    static void CalculateMaterialPropertySize(const BufferLayout& layout, uint32_t* outSize, uint32_t* outCount)
    {
        *outSize = layout.GetAlignedStride();
        *outCount = (uint32_t)layout.size();

        for (auto& element : layout)
        {
            if (PKAssets::GetElementIsResourceHandle(element.format))
            {
                // Reserve extra memory for actual texture references at the end of the block.
                *outSize += (uint32_t)sizeof(RHITexture*);
                (*outCount)++;
            }
        }
    }


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

        uint32_t serializedSize, serializedPropertyCount;
        CalculateMaterialSize(properties, keywords, &serializedSize, &serializedPropertyCount);
        InitializeShaderLayout(serializedSize, serializedPropertyCount);

        if (shadowShaderPathProp)
        {
            m_shadowShader = shadowShaderPathProp.as<ShaderAsset*>();
        }

        for (auto keyword : keywords)
        {
            Set<bool>(NameID(keyword.as<std::string>().c_str()), true);
        }

        for (auto property : properties)
        {
            auto propertyName = property.first.as<std::string>();
            auto type = property.second["Type"];

            if (!type)
            {
                continue;
            }

            auto nameId = NameID(propertyName.c_str());
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

    void Material::InitializeShaderLayout(uint32_t minSize, uint32_t minPropertyCount)
    {
        PK_THROW_ASSERT(m_shader->SupportsMaterials(), "Shader is doesn't support materials!");

        uint32_t materialSize, materialPropertyCount;
        CalculateMaterialPropertySize(m_shader->GetMaterialPropertyLayout(), &materialSize, &materialPropertyCount);

        materialSize = glm::max(materialSize, minSize);
        materialPropertyCount = glm::max(materialPropertyCount, minPropertyCount);

        ClearAndReserve(materialSize, materialPropertyCount);
        ReserveLayout(m_shader->GetMaterialPropertyLayout());

        auto builtIns = RHI::GetBuiltInResources();

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
bool PK::Asset::IsValidExtension<PK::Material>(const char* extension) { return strcmp(extension, ".material") == 0; }

template<>
PK::Ref<PK::Material> PK::Asset::Create() { return PK::CreateRef<Material>(); }
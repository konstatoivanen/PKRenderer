#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Yaml/ConvertMathTypes.h"
#include "Core/Yaml/ConvertTexture.h"
#include "Core/Yaml/ConvertShader.h"
#include "Rendering/RHI/GraphicsAPI.h"
#include "Rendering/RHI/BuiltInResources.h"
#include "Material.h"

using namespace PK::Math;
using namespace PK::Utilities;
using namespace PK::Core;
using namespace PK::Core::Services;
using namespace PK::Core::Assets;
using namespace PK::Rendering::Objects;
using namespace PK::Rendering::RHI;
using namespace PK::Rendering::RHI::Objects;

namespace PK::Rendering::Objects
{
    bool Material::SupportsKeyword(const uint32_t hashId) const { return m_shader->SupportsKeyword(hashId); }

    bool Material::SupportsKeywords(const uint32_t* hashIds, const uint32_t count) const { return m_shader->SupportsKeywords(hashIds, count); }

    void Material::CopyTo(char* dst, BindSet<Texture>* textureSet) const
    {
        auto& layout = m_shader->GetMaterialPropertyLayout();

        memcpy(dst, m_buffer, layout.GetAlignedStride());

        for (auto& element : layout)
        {
            switch (element.Type)
            {
                case ElementType::Texture2DHandle:
                    auto texIndex = textureSet->Set(*Get<Texture*>(element.NameHashId));
                    memcpy(dst + element.AlignedOffset, &texIndex, sizeof(int32_t));
                    break;
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

        m_shader = shaderPathProp.as<Shader*>();
        InitializeShaderLayout();

        if (shadowShaderPathProp)
        {
            m_shadowShader = shadowShaderPathProp.as<Shader*>();
        }

        if (keywords)
        {
            for (auto keyword : keywords)
            {
                Set<bool>(StringHashID::StringToID(keyword.as<std::string>()), true);
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

                auto nameHash = StringHashID::StringToID(propertyName);
                auto typeName = type.as<std::string>();
                auto elementType = PK::Assets::GetElementType(typeName.c_str());
                auto values = property.second["Value"];

                switch (elementType)
                {
                    case ElementType::Float: Set(nameHash, values.as<float>()); break;
                    case ElementType::Float2: Set(nameHash, values.as<float2>()); break;
                    case ElementType::Float3: Set(nameHash, values.as<float3>()); break;
                    case ElementType::Float4: Set(nameHash, values.as<float4>()); break;
                    case ElementType::Float2x2: Set(nameHash, values.as<float2x2>()); break;
                    case ElementType::Float3x3: Set(nameHash, values.as<float3x3>()); break;
                    case ElementType::Float4x4: Set(nameHash, values.as<float4x4>()); break;
                    case ElementType::Float3x4: Set(nameHash, values.as<float3x4>()); break;
                    case ElementType::Int: Set(nameHash, values.as<int>()); break;
                    case ElementType::Int2: Set(nameHash, values.as<int2>()); break;
                    case ElementType::Int3: Set(nameHash, values.as<int3>()); break;
                    case ElementType::Int4: Set(nameHash, values.as<int4>()); break;
                    case ElementType::Texture2DHandle: Set(nameHash, values.as<Texture*>()); break;
                    case ElementType::Texture3DHandle: Set(nameHash, values.as<Texture*>()); break;
                    case ElementType::TextureCubeHandle: Set(nameHash, values.as<Texture*>()); break;
                }
            }
        }
    }

    void Material::InitializeShaderLayout()
    {
        Clear();

        auto builtIns = GraphicsAPI::GetBuiltInResources();

        PK_THROW_ASSERT(m_shader->SupportsMaterials(), "Shader is doesn't support materials!");
        ReserveLayout(m_shader->GetMaterialPropertyLayout());

        for (auto& element : m_shader->GetMaterialPropertyLayout())
        {
            switch (element.Type)
            {
                case ElementType::Texture2DHandle: Set(element.NameHashId, builtIns->BlackTexture2D.get()); break;
            }
        }
    }
}

template<>
bool Asset::IsValidExtension<Material>(const std::string& extension) { return extension.compare(".material") == 0; }

template<>
Ref<Material> Asset::Create() { return CreateRef<Material>(); }
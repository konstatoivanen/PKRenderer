#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Utilities/FileIOBinary.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/TextureAsset.h"
#include "Core/Yaml/RapidyamlPrivate.h"
#include "Material.h"

namespace PK
{
    static void CalculateMaterialSize(const ryml::ConstNodeRef& properties, const ryml::ConstNodeRef& keywords, uint32_t* outSize, uint32_t* outCount)
    {
        *outSize = 0u;
        *outCount = 0u;

        if (keywords.readable())
        {
            *outSize += sizeof(bool) * keywords.num_children();
            *outCount += (uint32_t)keywords.num_children();
        }

        for (const auto property : properties.children())
        {
            auto type = property.find_child("Type");

            if (type.readable())
            {
                auto elementType = YAML::Read<ElementType>(type);

                *outSize += PKAssets::PKElementTypeToSize(elementType);
                (*outCount)++;

                if (RHIEnumConvert::IsResourceHandle(elementType))
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
            if (RHIEnumConvert::IsResourceHandle(element.format))
            {
                // Reserve extra memory for actual texture references at the end of the block.
                *outSize += (uint32_t)sizeof(RHITexture*);
                (*outCount)++;
            }
        }
    }


    size_t Material::GetPropertyStride() const { return m_shader->GetMaterialPropertyLayout().GetPaddedStride(); }

    bool Material::SupportsKeyword(const NameID keyword) const { return m_shader->SupportsKeyword(keyword); }

    bool Material::SupportsKeywords(const NameID* keywords, const uint32_t count) const { return m_shader->SupportsKeywords(keywords, count); }

    void Material::CopyTo(char* dst, BindSet<RHITexture>* textureSet) const
    {
        auto& layout = m_shader->GetMaterialPropertyLayout();

        memcpy(dst, GetByteBuffer(), layout.GetAlignedStride());

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
        void* fileData = nullptr;
        size_t fileSize = 0ull;

        if (FileIO::ReadBinary(filepath, &fileData, &fileSize) != 0)
        {
            PK_LOG_WARNING("Failed to read IYamlStruct at path '%'", filepath);
            return;
        }

        auto tree = ryml::parse_in_place(c4::substr(reinterpret_cast<char*>(fileData), fileSize));
        c4::yml::ConstNodeRef root = tree.rootref();

        auto material = root.find_child("Material");
        PK_THROW_ASSERT(material.readable(), "Could not locate material (%s) header in file.", filepath);

        auto shaderPathProp = material.find_child("Shader");
        auto shaderShadowPathProp = material.find_child("ShaderShadow");
        auto keywords = material.find_child("Keywords");
        auto properties = material.find_child("Properties");

        PK_THROW_ASSERT(shaderPathProp.readable(), "Material (%s) doesn't define a shader.", filepath);

        m_shader = YAML::Read<ShaderAsset*>(shaderPathProp);

        if (shaderShadowPathProp.readable())
        {
            m_shaderShadow = YAML::Read<ShaderAsset*>(shaderShadowPathProp);
        }

        uint32_t serializedSize, serializedPropertyCount;
        CalculateMaterialSize(properties, keywords, &serializedSize, &serializedPropertyCount);
        InitializeShaderLayout(serializedSize, serializedPropertyCount);

        if (keywords.readable())
        {
            for (auto keyword : keywords.children())
            {
                Set<bool>(NameID(YAML::Read<FixedString32>(keyword)), true);
            }
        }

        if (properties.readable())
        {
            for (auto property : properties.children())
            {
                auto type = property.find_child("Type");
                auto value = property.find_child("Value");

                if (type.readable() && value.readable())
                {
                    auto nameId = NameID(YAML::ReadKey<FixedString32>(property));
                    auto elementType = YAML::Read<ElementType>(type);

                    switch (elementType)
                    {
                        case ElementType::Float: Set(nameId, YAML::Read<float>(value)); break;
                        case ElementType::Float2: Set(nameId, YAML::Read<float2>(value)); break;
                        case ElementType::Float3: Set(nameId, YAML::Read<float3>(value)); break;
                        case ElementType::Float4: Set(nameId, YAML::Read<float4>(value)); break;
                        case ElementType::Float2x2: Set(nameId, YAML::Read<float2x2>(value)); break;
                        case ElementType::Float3x3: Set(nameId, YAML::Read<float3x3>(value)); break;
                        case ElementType::Float4x4: Set(nameId, YAML::Read<float3x4>(value)); break;
                        case ElementType::Float3x4: Set(nameId, YAML::Read<float4x4>(value)); break;
                        case ElementType::Int: Set(nameId, YAML::Read<int>(value)); break;
                        case ElementType::Int2: Set(nameId, YAML::Read<int2>(value)); break;
                        case ElementType::Int3: Set(nameId, YAML::Read<int3>(value)); break;
                        case ElementType::Int4: Set(nameId, YAML::Read<int4>(value)); break;
                        case ElementType::Texture2DHandle: Set(nameId, YAML::Read<TextureAsset*>(value)->GetRHI()); break;
                        case ElementType::Texture3DHandle: Set(nameId, YAML::Read<TextureAsset*>(value)->GetRHI()); break;
                        case ElementType::TextureCubeHandle: Set(nameId, YAML::Read<TextureAsset*>(value)); break;
                        default: PK_LOG_WARNING("Unsupported material parameter type"); break;
                    }
                }
            }
        }

        free(fileData);
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
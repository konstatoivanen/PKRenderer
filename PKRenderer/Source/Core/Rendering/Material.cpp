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
    Material::Material(const char* filepath)
    {
        void* fileData = nullptr;
        size_t fileSize = 0ull;

        if (FileIO::ReadBinary(filepath, false, &fileData, &fileSize) != 0)
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

        ReservePropertyBuffer();

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
                        case ElementType::Half:      Set(nameId, YAML::Read<ushort>(value)); break;
                        case ElementType::Half2:     Set(nameId, YAML::Read<ushort2>(value)); break;
                        case ElementType::Half3:     Set(nameId, YAML::Read<ushort3>(value)); break;
                        case ElementType::Half4:     Set(nameId, YAML::Read<ushort4>(value)); break;
                        case ElementType::Half2x2:   Set(nameId, YAML::Read<ushort2x2>(value)); break;
                        case ElementType::Half3x3:   Set(nameId, YAML::Read<ushort3x3>(value)); break;
                        case ElementType::Half4x4:   Set(nameId, YAML::Read<ushort4x4>(value)); break;

                        case ElementType::Float:     Set(nameId, YAML::Read<float>(value)); break;
                        case ElementType::Float2:    Set(nameId, YAML::Read<float2>(value)); break;
                        case ElementType::Float3:    Set(nameId, YAML::Read<float3>(value)); break;
                        case ElementType::Float4:    Set(nameId, YAML::Read<float4>(value)); break;
                        case ElementType::Float2x2:  Set(nameId, YAML::Read<float2x2>(value)); break;
                        case ElementType::Float3x3:  Set(nameId, YAML::Read<float3x3>(value)); break;
                        case ElementType::Float4x4:  Set(nameId, YAML::Read<float4x4>(value)); break;
                        case ElementType::Float3x4:  Set(nameId, YAML::Read<float3x4>(value)); break;

                        case ElementType::Double:    Set(nameId, YAML::Read<double>(value)); break;
                        case ElementType::Double2:   Set(nameId, YAML::Read<double2>(value)); break;
                        case ElementType::Double3:   Set(nameId, YAML::Read<double3>(value)); break;
                        case ElementType::Double4:   Set(nameId, YAML::Read<double4>(value)); break;
                        case ElementType::Double2x2: Set(nameId, YAML::Read<double2x2>(value)); break;
                        case ElementType::Double3x3: Set(nameId, YAML::Read<double3x3>(value)); break;
                        case ElementType::Double4x4: Set(nameId, YAML::Read<double4x4>(value)); break;

                        case ElementType::Int:       Set(nameId, YAML::Read<int>(value)); break;
                        case ElementType::Int2:      Set(nameId, YAML::Read<int2>(value)); break;
                        case ElementType::Int3:      Set(nameId, YAML::Read<int3>(value)); break;
                        case ElementType::Int4:      Set(nameId, YAML::Read<int4>(value)); break;

                        case ElementType::Uint:      Set(nameId, YAML::Read<uint>(value)); break;
                        case ElementType::Uint2:     Set(nameId, YAML::Read<uint2>(value)); break;
                        case ElementType::Uint3:     Set(nameId, YAML::Read<uint3>(value)); break;
                        case ElementType::Uint4:     Set(nameId, YAML::Read<uint4>(value)); break;

                        case ElementType::Short:     Set(nameId, YAML::Read<short>(value)); break;
                        case ElementType::Short2:    Set(nameId, YAML::Read<short2>(value)); break;
                        case ElementType::Short3:    Set(nameId, YAML::Read<short3>(value)); break;
                        case ElementType::Short4:    Set(nameId, YAML::Read<short4>(value)); break;

                        case ElementType::Ushort:    Set(nameId, YAML::Read<ushort>(value)); break;
                        case ElementType::Ushort2:   Set(nameId, YAML::Read<ushort2>(value)); break;
                        case ElementType::Ushort3:   Set(nameId, YAML::Read<ushort3>(value)); break;
                        case ElementType::Ushort4:   Set(nameId, YAML::Read<ushort4>(value)); break;

                        case ElementType::Long:      Set(nameId, YAML::Read<int64_t>(value)); break;
                        case ElementType::Long2:     Set(nameId, YAML::Read<long2>(value)); break;
                        case ElementType::Long3:     Set(nameId, YAML::Read<long3>(value)); break;
                        case ElementType::Long4:     Set(nameId, YAML::Read<long4>(value)); break;

                        case ElementType::Ulong:     Set(nameId, YAML::Read<ulong>(value)); break;
                        case ElementType::Ulong2:    Set(nameId, YAML::Read<ulong2>(value)); break;
                        case ElementType::Ulong3:    Set(nameId, YAML::Read<ulong3>(value)); break;
                        case ElementType::Ulong4:    Set(nameId, YAML::Read<ulong4>(value)); break;

                        case ElementType::Texture2DHandle: 
                        case ElementType::Texture3DHandle:
                        case ElementType::TextureCubeHandle: SetResource(nameId, YAML::Read<TextureAsset*>(value)->GetRHI()); break;

                        default: PK_LOG_WARNING("Unsupported material parameter type"); break;
                    }
                }
            }
        }

        free(fileData);
    }

    size_t Material::GetPropertyStride() const { return m_shader->GetMaterialPropertyLayout().GetStridePadded(); }

    bool Material::SupportsKeyword(const NameID keyword) const 
    {
        auto prop = m_shader->GetMaterialPropertyLayout().TryGetElement(keyword);
        return prop && prop->format == ElementType::Keyword;
    }

    void Material::CopyTo(char* dst, RHITextureBindSet* textureSet) const
    {
        auto& layout = m_shader->GetMaterialPropertyLayout();

        memcpy(dst, m_propertyBuffer.GetData(), layout.GetStride());

        for (auto& element : layout)
        {
            switch (element->format)
            {
                case ElementType::Texture2DHandle:
                {
                    auto texIndex = textureSet->Add(GetResource<RHITexture>(element->name));
                    memcpy(dst + element->offset, &texIndex, sizeof(int32_t));
                }
                break;
                default: break;
            }
        }
    }

    void Material::ReservePropertyBuffer()
    {
        PK_THROW_ASSERT(m_shader->SupportsMaterials(), "Shader is doesn't support materials!");

        EndWrite();

        auto& layout = m_shader->GetMaterialPropertyLayout();
        m_propertyBuffer.Reserve(layout.GetStrideMaterial());

        BeginWrite(&layout, m_propertyBuffer.GetData());

        auto builtIns = RHI::GetBuiltInResources();

        for (auto& element : m_shader->GetMaterialPropertyLayout())
        {
            switch (element->format)
            {
                case ElementType::Texture2DHandle: SetResource(element->name, builtIns->BlackTexture2D.get()); break;
                default: break;
            }
        }
    }
}

template<>
bool PK::Asset::IsValidExtension<PK::Material>(const char* extension) { return strcmp(extension, ".material") == 0; }

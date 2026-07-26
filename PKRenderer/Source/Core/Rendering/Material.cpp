#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Utilities/FileIO.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/TextureAsset.h"
#include "Core/Yaml/Serialize.h"
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

        auto tree = ryml::parse_in_place(c4::substr(static_cast<char*>(fileData), fileSize));
        c4::yml::ConstNodeRef root = tree.rootref();

        auto material = root.find_child("Material");
        PK_FATAL_ASSERT(material.readable(), "Could not locate material (%s) header in file.", filepath);

        auto shaderPathProp = material.find_child("Shader");
        auto shaderShadowPathProp = material.find_child("ShaderShadow");
        auto keywords = material.find_child("Keywords");
        auto properties = material.find_child("Properties");

        PK_FATAL_ASSERT(shaderPathProp.readable(), "Material (%s) doesn't define a shader.", filepath);

        m_shader = Serialize::Read<ShaderAsset*>(shaderPathProp);

        if (shaderShadowPathProp.readable())
        {
            m_shaderShadow = Serialize::Read<ShaderAsset*>(shaderShadowPathProp);
        }

        ReservePropertyBuffer();

        if (keywords.readable())
        {
            for (auto keyword : keywords.children())
            {
                Set<bool>(NameID(Serialize::Read<FixedString32>(keyword)), true);
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
                    auto nameId = NameID(Serialize::ReadKey<FixedString32>(property));
                    auto elementType = Serialize::Read<ElementType>(type);

                    switch (elementType)
                    {
                        case ElementType::Half:      Set(nameId, Serialize::Read<ushort>(value)); break;
                        case ElementType::Half2:     Set(nameId, Serialize::Read<ushort2>(value)); break;
                        case ElementType::Half3:     Set(nameId, Serialize::Read<ushort3>(value)); break;
                        case ElementType::Half4:     Set(nameId, Serialize::Read<ushort4>(value)); break;
                        case ElementType::Half2x2:   Set(nameId, Serialize::Read<ushort2x2>(value)); break;
                        case ElementType::Half3x3:   Set(nameId, Serialize::Read<ushort3x3>(value)); break;
                        case ElementType::Half4x4:   Set(nameId, Serialize::Read<ushort4x4>(value)); break;

                        case ElementType::Float:     Set(nameId, Serialize::Read<float>(value)); break;
                        case ElementType::Float2:    Set(nameId, Serialize::Read<float2>(value)); break;
                        case ElementType::Float3:    Set(nameId, Serialize::Read<float3>(value)); break;
                        case ElementType::Float4:    Set(nameId, Serialize::Read<float4>(value)); break;
                        case ElementType::Float2x2:  Set(nameId, Serialize::Read<float2x2>(value)); break;
                        case ElementType::Float3x3:  Set(nameId, Serialize::Read<float3x3>(value)); break;
                        case ElementType::Float4x4:  Set(nameId, Serialize::Read<float4x4>(value)); break;
                        case ElementType::Float3x4:  Set(nameId, Serialize::Read<float3x4>(value)); break;

                        case ElementType::Double:    Set(nameId, Serialize::Read<double>(value)); break;
                        case ElementType::Double2:   Set(nameId, Serialize::Read<double2>(value)); break;
                        case ElementType::Double3:   Set(nameId, Serialize::Read<double3>(value)); break;
                        case ElementType::Double4:   Set(nameId, Serialize::Read<double4>(value)); break;
                        case ElementType::Double2x2: Set(nameId, Serialize::Read<double2x2>(value)); break;
                        case ElementType::Double3x3: Set(nameId, Serialize::Read<double3x3>(value)); break;
                        case ElementType::Double4x4: Set(nameId, Serialize::Read<double4x4>(value)); break;

                        case ElementType::Int:       Set(nameId, Serialize::Read<int>(value)); break;
                        case ElementType::Int2:      Set(nameId, Serialize::Read<int2>(value)); break;
                        case ElementType::Int3:      Set(nameId, Serialize::Read<int3>(value)); break;
                        case ElementType::Int4:      Set(nameId, Serialize::Read<int4>(value)); break;

                        case ElementType::Uint:      Set(nameId, Serialize::Read<uint>(value)); break;
                        case ElementType::Uint2:     Set(nameId, Serialize::Read<uint2>(value)); break;
                        case ElementType::Uint3:     Set(nameId, Serialize::Read<uint3>(value)); break;
                        case ElementType::Uint4:     Set(nameId, Serialize::Read<uint4>(value)); break;

                        case ElementType::Short:     Set(nameId, Serialize::Read<short>(value)); break;
                        case ElementType::Short2:    Set(nameId, Serialize::Read<short2>(value)); break;
                        case ElementType::Short3:    Set(nameId, Serialize::Read<short3>(value)); break;
                        case ElementType::Short4:    Set(nameId, Serialize::Read<short4>(value)); break;

                        case ElementType::Ushort:    Set(nameId, Serialize::Read<ushort>(value)); break;
                        case ElementType::Ushort2:   Set(nameId, Serialize::Read<ushort2>(value)); break;
                        case ElementType::Ushort3:   Set(nameId, Serialize::Read<ushort3>(value)); break;
                        case ElementType::Ushort4:   Set(nameId, Serialize::Read<ushort4>(value)); break;

                        case ElementType::Long:      Set(nameId, Serialize::Read<int64_t>(value)); break;
                        case ElementType::Long2:     Set(nameId, Serialize::Read<long2>(value)); break;
                        case ElementType::Long3:     Set(nameId, Serialize::Read<long3>(value)); break;
                        case ElementType::Long4:     Set(nameId, Serialize::Read<long4>(value)); break;

                        case ElementType::Ulong:     Set(nameId, Serialize::Read<ulong>(value)); break;
                        case ElementType::Ulong2:    Set(nameId, Serialize::Read<ulong2>(value)); break;
                        case ElementType::Ulong3:    Set(nameId, Serialize::Read<ulong3>(value)); break;
                        case ElementType::Ulong4:    Set(nameId, Serialize::Read<ulong4>(value)); break;

                        case ElementType::Texture2DHandle: 
                        case ElementType::Texture3DHandle:
                        case ElementType::TextureCubeHandle: SetResource(nameId, Serialize::Read<TextureAsset*>(value)->GetRHI()); break;

                        default: PK_LOG_WARNING("Unsupported material parameter type"); break;
                    }
                }
            }
        }

        Memory::Free(fileData);
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

        for (const auto& element : layout)
        {
            switch (element.format)
            {
                case ElementType::Texture2DHandle:
                {
                    auto texIndex = textureSet->Add(GetResource<RHITexture>(element.name));
                    memcpy(dst + element.offset, &texIndex, sizeof(int32_t));
                }
                break;
                default: break;
            }
        }
    }

    void Material::ReservePropertyBuffer()
    {
        PK_FATAL_ASSERT(m_shader->SupportsMaterials(), "Shader is doesn't support materials!");

        EndWrite();

        auto& layout = m_shader->GetMaterialPropertyLayout();
        m_propertyBuffer.Reserve(layout.GetStrideMaterial(), false);

        BeginWrite(&layout, m_propertyBuffer.GetData());

        auto builtIns = RHI::GetBuiltInResources();

        for (const auto& element : m_shader->GetMaterialPropertyLayout())
        {
            switch (element.format)
            {
                case ElementType::Texture2DHandle: SetResource(element.name, builtIns->BlackTexture2D.get()); break;
                default: break;
            }
        }
    }
}

template<>
const char* PK::Asset::GetExtension<PK::Material>() { return "*.material"; }

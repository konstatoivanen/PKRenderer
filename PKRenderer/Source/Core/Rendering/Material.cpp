#include "PrecompiledHeader.h"
#include "Core/Base/FileIO.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/TextureAsset.h"
#include "Core/Serialization/Serialize.h"
#include "Material.h"

namespace PK
{
    Material::Material(const char* filepath)
    {
        void* fileData = nullptr;
        size_t fileSize = 0ull;

        if (FileIO::Read(filepath, false, &fileData, &fileSize) != 0)
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

        m_shader = Serialize::ReadVal<ShaderAsset*>(shaderPathProp);

        if (shaderShadowPathProp.readable())
        {
            m_shaderShadow = Serialize::ReadVal<ShaderAsset*>(shaderShadowPathProp);
        }

        ReservePropertyBuffer();

        if (keywords.readable())
        {
            for (auto keyword : keywords.children())
            {
                Set<bool>(NameID(Serialize::ReadVal<FixedString32>(keyword)), true);
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
                    auto uniform = Serialize::ReadVal<Uniform>(type);

                    switch (uniform)
                    {
                        case Uniform::Bool: Set(nameId, Serialize::ReadVal<bool>(value)); break;
                        case Uniform::Bool2: Set(nameId, Serialize::ReadVal<bool2>(value)); break;
                        case Uniform::Bool3: Set(nameId, Serialize::ReadVal<bool3>(value)); break;
                        case Uniform::Bool4: Set(nameId, Serialize::ReadVal<bool4>(value)); break;

                        case Uniform::Byte: Set(nameId, Serialize::ReadVal<byte>(value)); break;
                        case Uniform::Byte2: Set(nameId, Serialize::ReadVal<byte2>(value)); break;
                        case Uniform::Byte3: Set(nameId, Serialize::ReadVal<byte3>(value)); break;
                        case Uniform::Byte4: Set(nameId, Serialize::ReadVal<byte4>(value)); break;

                        case Uniform::Sbyte: Set(nameId, Serialize::ReadVal<sbyte>(value)); break;
                        case Uniform::Sbyte2: Set(nameId, Serialize::ReadVal<sbyte2>(value)); break;
                        case Uniform::Sbyte3: Set(nameId, Serialize::ReadVal<sbyte3>(value)); break;
                        case Uniform::Sbyte4: Set(nameId, Serialize::ReadVal<sbyte4>(value)); break;

                        case Uniform::Short: Set(nameId, Serialize::ReadVal<short>(value)); break;
                        case Uniform::Short2: Set(nameId, Serialize::ReadVal<short2>(value)); break;
                        case Uniform::Short3: Set(nameId, Serialize::ReadVal<short3>(value)); break;
                        case Uniform::Short4: Set(nameId, Serialize::ReadVal<short4>(value)); break;
                        case Uniform::Ushort: Set(nameId, Serialize::ReadVal<ushort>(value)); break;

                        case Uniform::Ushort2: Set(nameId, Serialize::ReadVal<ushort2>(value)); break;
                        case Uniform::Ushort3: Set(nameId, Serialize::ReadVal<ushort3>(value)); break;
                        case Uniform::Ushort4: Set(nameId, Serialize::ReadVal<ushort4>(value)); break;

                        case Uniform::Half: Set(nameId, Serialize::ReadVal<ushort>(value)); break;
                        case Uniform::Half2: Set(nameId, Serialize::ReadVal<ushort2>(value)); break;
                        case Uniform::Half3: Set(nameId, Serialize::ReadVal<ushort3>(value)); break;
                        case Uniform::Half4: Set(nameId, Serialize::ReadVal<ushort4>(value)); break;

                        case Uniform::Float: Set(nameId, Serialize::ReadVal<float>(value)); break;
                        case Uniform::Float2: Set(nameId, Serialize::ReadVal<float2>(value)); break;
                        case Uniform::Float3: Set(nameId, Serialize::ReadVal<float3>(value)); break;
                        case Uniform::Float4: Set(nameId, Serialize::ReadVal<float4>(value)); break;

                        case Uniform::Int: Set(nameId, Serialize::ReadVal<int>(value)); break;
                        case Uniform::Int2: Set(nameId, Serialize::ReadVal<int2>(value)); break;
                        case Uniform::Int3: Set(nameId, Serialize::ReadVal<int3>(value)); break;
                        case Uniform::Int4: Set(nameId, Serialize::ReadVal<int4>(value)); break;

                        case Uniform::Uint: Set(nameId, Serialize::ReadVal<uint>(value)); break;
                        case Uniform::Uint2: Set(nameId, Serialize::ReadVal<uint2>(value)); break;
                        case Uniform::Uint3: Set(nameId, Serialize::ReadVal<uint3>(value)); break;
                        case Uniform::Uint4: Set(nameId, Serialize::ReadVal<uint4>(value)); break;

                        case Uniform::Long: Set(nameId, Serialize::ReadVal<int64_t>(value)); break;
                        case Uniform::Long2: Set(nameId, Serialize::ReadVal<long2>(value)); break;
                        case Uniform::Long3: Set(nameId, Serialize::ReadVal<long3>(value)); break;
                        case Uniform::Long4: Set(nameId, Serialize::ReadVal<long4>(value)); break;

                        case Uniform::Ulong: Set(nameId, Serialize::ReadVal<uint64_t>(value)); break;
                        case Uniform::Ulong2: Set(nameId, Serialize::ReadVal<ulong2>(value)); break;
                        case Uniform::Ulong3: Set(nameId, Serialize::ReadVal<ulong3>(value)); break;
                        case Uniform::Ulong4: Set(nameId, Serialize::ReadVal<ulong4>(value)); break;

                        case Uniform::Double: Set(nameId, Serialize::ReadVal<double>(value)); break;
                        case Uniform::Double2: Set(nameId, Serialize::ReadVal<double2>(value)); break;
                        case Uniform::Double3: Set(nameId, Serialize::ReadVal<double3>(value)); break;
                        case Uniform::Double4: Set(nameId, Serialize::ReadVal<double4>(value)); break;

                        case Uniform::Float2x2:  Set(nameId, Serialize::ReadVal<float2x2>(value)); break;
                        case Uniform::Float2x3:  Set(nameId, Serialize::ReadVal<float2x3>(value)); break;
                        case Uniform::Float2x4:  Set(nameId, Serialize::ReadVal<float2x4>(value)); break;
                        case Uniform::Float3x2:  Set(nameId, Serialize::ReadVal<float3x2>(value)); break;
                        case Uniform::Float3x3:  Set(nameId, Serialize::ReadVal<float3x3>(value)); break;
                        case Uniform::Float3x4:  Set(nameId, Serialize::ReadVal<float3x4>(value)); break;
                        case Uniform::Float4x2:  Set(nameId, Serialize::ReadVal<float4x2>(value)); break;
                        case Uniform::Float4x3:  Set(nameId, Serialize::ReadVal<float4x3>(value)); break;
                        case Uniform::Float4x4:  Set(nameId, Serialize::ReadVal<float4x4>(value)); break;

                        case Uniform::Double2x2: Set(nameId, Serialize::ReadVal<double2x2>(value)); break;
                        case Uniform::Double3x3: Set(nameId, Serialize::ReadVal<double3x3>(value)); break;
                        case Uniform::Double4x4: Set(nameId, Serialize::ReadVal<double4x4>(value)); break;

                        case Uniform::Half2x2:   Set(nameId, Serialize::ReadVal<ushort2x2>(value)); break;
                        case Uniform::Half3x3:   Set(nameId, Serialize::ReadVal<ushort3x3>(value)); break;
                        case Uniform::Half4x4:   Set(nameId, Serialize::ReadVal<ushort4x4>(value)); break;

                        case Uniform::Texture2D:
                        case Uniform::Texture3D:
                        case Uniform::TextureCube: SetResource(nameId, Serialize::ReadVal<TextureAsset*>(value)->GetRHI()); break;

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
        return prop && prop->format == Uniform::Keyword;
    }

    void Material::CopyTo(char* dst) const
    {
        auto& layout = m_shader->GetMaterialPropertyLayout();

        memcpy(dst, m_propertyBuffer.GetData(), layout.GetStride());

        for (const auto& element : layout)
        {
            switch (element.format)
            {
                case Uniform::Texture2D:
                {
                    auto texIndex = RHI::GetGraph()->MakeResident(GetResource<RHITexture>(element.name));
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
                case Uniform::Texture2D: SetResource(element.name, builtIns->BlackTexture2D.get()); break;
                default: break;
            }
        }
    }
}

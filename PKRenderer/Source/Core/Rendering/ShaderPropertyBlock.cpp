#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Math/FunctionsMisc.h"
#include "ShaderPropertyBlock.h"

namespace PK
{
    ShaderPropertyLayout::ShaderPropertyLayout(ShaderProperty* elements, size_t count) : TBase(count, 1u)
    {
        for (auto i = 0u; i < count; ++i)
        {
            Add(elements[i]);
        }

        CalculateOffsetsAndStride();
    }

    ShaderPropertyLayout::ShaderPropertyLayout(std::initializer_list<ShaderProperty> elements) : TBase(elements.size(), 1u)
    {
        for (auto& element : elements)
        {
            Add(element);
        }

        CalculateOffsetsAndStride();
    }

    const ShaderProperty* ShaderPropertyLayout::TryGetElement(NameID name) const
    {
        auto valueIndex = GetHashIndex((size_t)name.identifier);

        if (valueIndex != -1)
        {
            return &(*this)[valueIndex];
        }

        return nullptr;
    }

    void ShaderPropertyLayout::CalculateOffsetsAndStride()
    {
        m_hash = 0ull;
        m_stride = 0u;
        m_stridePadded = 0u;
        auto maxAlignment = 0u;

        for (auto i = 0u; i < GetCount(); ++i)
        {
            auto& element = (*this)[i];

            if (element.format != ElementType::Keyword)
            {
                auto alignment = RHIEnumConvert::Alignment(element.format);

                m_stride = Math::GetAlignedSize(m_stride, alignment);
                element.offset = m_stride;
                element.offsetHandle = 0u;
                m_stride += element.GetSize();

                if (alignment > maxAlignment)
                {
                    maxAlignment = alignment;
                }
            }
        }

        // As per std140 a structure has a base alignment equal to the largest base alignment of any of its members, rounded up to a multiple of 16.
        maxAlignment = Math::GetAlignedSize(maxAlignment, 16u);
        m_stridePadded = Math::GetAlignedSize(m_stride, maxAlignment);
        m_hash = Hash::FNV1AHash(m_values, GetCount() * sizeof(ShaderProperty));

        // Material binding specific layout info.
        auto keywordsStride = m_stridePadded;

        for (auto i = 0u; i < GetCount(); ++i)
        {
            auto& element = (*this)[i];

            if (element.format == ElementType::Keyword)
            {
                keywordsStride = Math::GetAlignedSize(keywordsStride, 1u);
                element.offset = keywordsStride;
                keywordsStride += 1ull;
            }
        }

        m_strideMaterial = keywordsStride;

        for (auto i = 0u; i < GetCount(); ++i)
        {
            auto& element = (*this)[i];

            if (RHIEnumConvert::IsResourceHandle(element.format))
            {
                m_strideMaterial = Math::GetAlignedSize(m_strideMaterial, 8u);
                element.offsetHandle = m_strideMaterial;
                m_strideMaterial += 8ull;
            }
        }
    }


    void ShaderPropertyWriter::BeginWrite(const ShaderPropertyLayout* layout, void* memory)
    {
        m_layout = layout;
        m_memory = reinterpret_cast<uint8_t*>(memory);
    }

    void* ShaderPropertyWriter::EndWrite()
    {
        auto memory = m_memory;
        m_layout = nullptr;
        m_memory = nullptr;
        return memory;
    }

    bool ShaderPropertyWriter::Write(NameID name, const void* value, size_t size)
    {
        if (m_layout && m_memory)
        {
            auto prop = m_layout->TryGetElement(name);

            if (prop && prop->GetSize() >= size)
            {
                memcpy(m_memory + prop->offset, value, size);
                return true;
            }
        }

        PK_DEBUG_WARNING("Warning trying to set a property '%s' that doesn't exist in layout!", name.c_str());
        return false;
    }

    bool ShaderPropertyWriter::WriteResource(NameID name, void* value)
    {
        if (m_layout && m_memory)
        {
            auto prop = m_layout->TryGetElement(name);

            if (prop && RHIEnumConvert::IsResourceHandle(prop->format))
            {
                memcpy(m_memory + prop->offsetHandle, &value, sizeof(void*));
                return true;
            }
        }

        PK_DEBUG_WARNING("Warning trying to set a resource '%s' that doesn't exist in layout!", name.c_str());
        return false;
    }

    const void* ShaderPropertyWriter::Read(NameID name) const
    {
        if (m_layout && m_memory)
        {
            auto prop = m_layout->TryGetElement(name);

            if (prop)
            {
                return m_memory + prop->offset;
            }
        }

        PK_DEBUG_WARNING("Warning trying to get a property '%s' that doesn't exist in layout!", name.c_str());
        return nullptr;
    }

    const void* ShaderPropertyWriter::ReadResource(NameID name) const
    {
        if (m_layout && m_memory)
        {
            auto prop = m_layout->TryGetElement(name);

            if (prop && RHIEnumConvert::IsResourceHandle(prop->format))
            {
                return m_memory + prop->offsetHandle;
            }
        }

        PK_DEBUG_WARNING("Warning trying to get a resource '%s' that doesn't exist in layout!", name.c_str());
        return nullptr;
    }


    ShaderPropertyBlock::~ShaderPropertyBlock() = default;

    void ShaderPropertyBlock::ReserveLayout(const ShaderPropertyLayout& layout)
    {
        for (auto& element : layout)
        {
            switch (element->format)
            {
                case ElementType::Half:      Reserve<ushort>(element->name, element->count); break;
                case ElementType::Half2:     Reserve<ushort2>(element->name, element->count); break;
                case ElementType::Half3:     Reserve<ushort3>(element->name, element->count); break;
                case ElementType::Half4:     Reserve<ushort4>(element->name, element->count); break;
                case ElementType::Half2x2:   Reserve<ushort2x2>(element->name, element->count); break;
                case ElementType::Half3x3:   Reserve<ushort3x3>(element->name, element->count); break;
                case ElementType::Half4x4:   Reserve<ushort4x4>(element->name, element->count); break;

                case ElementType::Float:     Reserve<float>(element->name, element->count); break;
                case ElementType::Float2:    Reserve<float2>(element->name, element->count); break;
                case ElementType::Float3:    Reserve<float3>(element->name, element->count); break;
                case ElementType::Float4:    Reserve<float4>(element->name, element->count); break;
                case ElementType::Float2x2:  Reserve<float2x2>(element->name, element->count); break;
                case ElementType::Float3x3:  Reserve<float3x3>(element->name, element->count); break;
                case ElementType::Float4x4:  Reserve<float4x4>(element->name, element->count); break;
                case ElementType::Float3x4:  Reserve<float3x4>(element->name, element->count); break;

                case ElementType::Double:    Reserve<double>(element->name, element->count); break;
                case ElementType::Double2:   Reserve<double2>(element->name, element->count); break;
                case ElementType::Double3:   Reserve<double3>(element->name, element->count); break;
                case ElementType::Double4:   Reserve<double4>(element->name, element->count); break;
                case ElementType::Double2x2: Reserve<double2x2>(element->name, element->count); break;
                case ElementType::Double3x3: Reserve<double3x3>(element->name, element->count); break;
                case ElementType::Double4x4: Reserve<double4x4>(element->name, element->count); break;

                case ElementType::Int:       Reserve<int>(element->name, element->count); break;
                case ElementType::Int2:      Reserve<int2>(element->name, element->count); break;
                case ElementType::Int3:      Reserve<int3>(element->name, element->count); break;
                case ElementType::Int4:      Reserve<int4>(element->name, element->count); break;

                case ElementType::Uint:      Reserve<uint>(element->name, element->count); break;
                case ElementType::Uint2:     Reserve<uint2>(element->name, element->count); break;
                case ElementType::Uint3:     Reserve<uint3>(element->name, element->count); break;
                case ElementType::Uint4:     Reserve<uint4>(element->name, element->count); break;

                case ElementType::Short:     Reserve<short>(element->name, element->count); break;
                case ElementType::Short2:    Reserve<short2>(element->name, element->count); break;
                case ElementType::Short3:    Reserve<short3>(element->name, element->count); break;
                case ElementType::Short4:    Reserve<short4>(element->name, element->count); break;

                case ElementType::Ushort:    Reserve<ushort>(element->name, element->count); break;
                case ElementType::Ushort2:   Reserve<ushort2>(element->name, element->count); break;
                case ElementType::Ushort3:   Reserve<ushort3>(element->name, element->count); break;
                case ElementType::Ushort4:   Reserve<ushort4>(element->name, element->count); break;

                case ElementType::Long:      Reserve<int64_t>(element->name, element->count); break;
                case ElementType::Long2:     Reserve<long2>(element->name, element->count); break;
                case ElementType::Long3:     Reserve<long3>(element->name, element->count); break;
                case ElementType::Long4:     Reserve<long4>(element->name, element->count); break;

                case ElementType::Ulong:     Reserve<ulong>(element->name, element->count); break;
                case ElementType::Ulong2:    Reserve<ulong2>(element->name, element->count); break;
                case ElementType::Ulong3:    Reserve<ulong3>(element->name, element->count); break;
                case ElementType::Ulong4:    Reserve<ulong4>(element->name, element->count); break;

                case ElementType::Texture2DHandle:
                case ElementType::Texture3DHandle:
                case ElementType::TextureCubeHandle: Reserve<uint>(element->name, element->count); break;

                default: PK_LOG_WARNING("Trying to append an unsupported type to shader propertyblock!");
            }
        }
    }
}

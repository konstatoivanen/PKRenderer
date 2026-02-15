#include "PrecompiledHeader.h"
#include "Core/Utilities/Hash.h"
#include "Core/Math/FunctionsMisc.h"
#include "Layout.h"

namespace PK
{
    void ShaderStructLayout::CalculateOffsetsAndStride()
    {
        m_hash = 0ull;
        m_stride = 0u;
        m_stridePadded = 0u;
        auto maxAlignment = 0u;
        auto elements = data();
        auto count = size();

        for (auto i = 0u; i < count; ++i)
        {
            auto& element = elements[i];
            auto alignment = RHIEnumConvert::Alignment(element.format);

            m_stride = Math::GetAlignedSize(m_stride, alignment);
            element.offset = m_stride;
            m_stride += element.GetSize();

            if (alignment > maxAlignment)
            {
                maxAlignment = alignment;
            }
        }

        // As per std140 a structure has a base alignment equal to the largest base alignment of any of its members, rounded up to a multiple of 16.
        maxAlignment = Math::GetAlignedSize(maxAlignment, 16u);
        m_stridePadded = Math::GetAlignedSize(m_stride, maxAlignment);
        m_hash = Hash::FNV1AHash(elements, count * sizeof(ShaderStructElement));
    }

    const ShaderResourceElement* ShaderResourceLayout::TryGetElement(NameID name, uint32_t* index) const
    {
        // This will always have a small amount of elements (max 16) so mapping is redundant
        // Additionally this is only used for debug.
        for (auto i = 0u; i < GetCount(); ++i)
        {
            if (GetData()[i].name == name)
            {
                *index = i;
                return GetData() + i;
            }
        }

        return nullptr;
    }

    const ShaderVertexInputElement* ShaderVertexInputLayout::TryGetElement(NameID name, uint32_t* index) const
    {
        auto valueIndex = GetHashIndex((size_t)name.identifier);

        if (valueIndex != -1)
        {
            *index = (uint32_t)valueIndex;
            return &(*this)[valueIndex];
        }

        return nullptr;
    }

    void VertexStreamLayout::Add(const VertexStreamLayout& other)
    {
        for (auto& element : other)
        {
            Add(*element);
        }

        CalculateOffsetsAndStride();
    }

    void VertexStreamLayout::Add(const VertexStreamLayout& other, uint32_t stream)
    {
        for (auto& element : other)
        {
            Add(*element)->stream = stream;
        }

        CalculateOffsetsAndStride();
    }

    void VertexStreamLayout::CalculateOffsetsAndStride()
    {
        auto data = GetData();

        memset(m_streamStrides, 0, sizeof(m_streamStrides));

        for (auto i = 0u; i < GetCount(); ++i)
        {
            data[i].offset = m_streamStrides[data[i].stream];
            m_streamStrides[data[i].stream] += data[i].size;
        }

        for (auto i = 0u; i < GetCount(); ++i)
        {
            data[i].stride = m_streamStrides[data[i].stream];
        }

        m_totalStride = 0u;

        for (auto i = 0u; i < PK_RHI_MAX_VERTEX_ATTRIBUTES; ++i)
        {
            m_totalStride += m_streamStrides[i];
        }
    }
}

#include "PrecompiledHeader.h"
#include "Utilities/Hash.h"
#include "Layout.h"

namespace PK::Rendering::RHI
{
    PushConstantLayout::PushConstantLayout(const PushConstant* variables, size_t count)
    {
        for (auto i = 0u; i < count; ++i)
        {
            (*this)[variables[i].name] = variables[i];
        }
    }

    const PushConstant* PushConstantLayout::TryGetElement(Utilities::NameID name) const
    {
        auto iter = find(name);
        return iter != end() ? &iter->second : nullptr;
    }

    const ResourceElement* ResourceLayout::TryGetElement(Utilities::NameID name, uint32_t* index) const
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

    void BufferLayout::CalculateOffsetsAndStride(bool applyOffsets)
    {
        m_hash = 0ull;
        m_stride = 0;
        m_alignedStride = 0;
        m_paddedStride = 0;
        auto maxAlignment = 0u;
        auto* elements = data();

        for (auto i = 0u; i < size(); ++i)
        {
            auto element = elements + i;
            auto alignment = ElementConvert::Alignment(element->type);

            if (applyOffsets)
            {
                element->offset = m_stride;
            }

            m_stride += element->GetSize();

            m_alignedStride = alignment * (uint32_t)glm::ceil(m_alignedStride / (float)alignment);

            if (applyOffsets)
            {
                element->alignedOffset = m_alignedStride;
            }

            m_alignedStride += element->GetSize();

            if (alignment > maxAlignment)
            {
                maxAlignment = alignment;
            }

            m_elementMap[element->name] = i;
        }

        // As per std140 a structure has a base alignment equal to the largest base alignment of any of its members, rounded up to a multiple of 16.
        maxAlignment = 16 * (uint32_t)glm::ceil(maxAlignment / 16.0f);
        m_paddedStride = maxAlignment * (uint32_t)glm::ceil(m_alignedStride / (float)maxAlignment);
        m_hash = Utilities::Hash::FNV1AHash(data(), size() * sizeof(BufferElement));
    }

    const BufferElement* BufferLayout::TryGetElement(Utilities::NameID name, uint32_t* index) const
    {
        auto iterator = m_elementMap.find(name);

        if (iterator != m_elementMap.end())
        {
            *index = iterator->second;
            return (data() + iterator->second);
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

        for (auto i = 0u; i < PK_MAX_VERTEX_ATTRIBUTES; ++i)
        {
            m_totalStride += m_streamStrides[i];
        }
    }
}

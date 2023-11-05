#include "PrecompiledHeader.h"
#include "Layout.h"

namespace PK::Rendering::RHI
{
    void BufferLayout::CalculateOffsetsAndStride(bool applyOffsets)
    {
        m_stride = 0;
        m_alignedStride = 0;
        m_paddedStride = 0;
        auto maxAlignment = 0u;
        auto* elements = data();

        for (auto i = 0u; i < size(); ++i)
        {
            auto element = elements + i;
            auto alignment = ElementConvert::Alignment(element->Type);

            if (applyOffsets)
            {
                element->Offset = m_stride;
            }

            m_stride += element->Size();

            m_alignedStride = alignment * (uint32_t)glm::ceil(m_alignedStride / (float)alignment);

            if (applyOffsets)
            {
                element->AlignedOffset = m_alignedStride;
            }

            m_alignedStride += element->Size();

            if (alignment > maxAlignment)
            {
                maxAlignment = alignment;
            }

            m_elementMap[element->NameHashId] = i;
        }

        // As per std140 a structure has a base alignment equal to the largest base alignment of any of its members, rounded up to a multiple of 16.
        maxAlignment = 16 * (uint32_t)glm::ceil(maxAlignment / 16.0f);
        m_paddedStride = maxAlignment * (uint32_t)glm::ceil(m_alignedStride / (float)maxAlignment);
    }

    const BufferElement* BufferLayout::TryGetElement(uint32_t nameHashId, uint32_t* index) const
    {
        auto iterator = m_elementMap.find(nameHashId);

        if (iterator != m_elementMap.end())
        {
            *index = iterator->second;
            return (data() + iterator->second);
        }

        return nullptr;
    }

    const ResourceElement* ResourceLayout::TryGetElement(uint32_t nameHashId, uint32_t* index) const
    {
        // This will always have a small amount of elements (max 16) so mapping is redundant
        // Additionally this is only used for debug.
        for (auto i = 0u; i < GetCount(); ++i)
        {
            if (GetData()[i].NameHashId == nameHashId)
            {
                *index = i;
                return GetData() + i;
            }
        }
        
        return nullptr;
    }

    const ConstantVariable* ConstantBufferLayout::TryGetElement(uint32_t nameHashId) const
    {
        if (count(nameHashId) > 0)
        {
            return &at(nameHashId);
        }

        return nullptr;
    }

    void ConstantBufferLayout::FillElementMap(const ConstantVariable* variables, size_t count)
    {
        for (auto i = 0u; i < count; ++i)
        {
            (*this)[variables[i].NameHashId] = variables[i];
        }
    }
}

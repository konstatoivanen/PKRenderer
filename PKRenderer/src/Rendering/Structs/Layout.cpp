#include "PrecompiledHeader.h"
#include "Layout.h"
#include "Rendering/Structs/Enums.h"

namespace PK::Rendering::Structs
{
    void BufferLayout::CalculateOffsetsAndStride()
    {
        m_stride = 0;
        m_alignedStride = 0;
        m_paddedStride = 0;
        auto maxAlignment = 0u;

        for (auto& element : m_elements)
        {
            auto alignment = ElementConvert::Alignment(element.Type);

            element.Offset = m_stride;
            m_stride += element.Size;
            
            m_alignedStride = alignment * (uint)glm::ceil(m_alignedStride / (float)alignment);

            element.AlignedOffset = m_alignedStride;
            m_alignedStride += element.Size;

            if (alignment > maxAlignment)
            {
                maxAlignment = alignment;
            }
        }

        // As per std140 a structure has a base alignment equal to the largest base alignment of any of its members, rounded up to a multiple of 16.
        maxAlignment = 16 * (uint)glm::ceil(maxAlignment / 16.0f);
        m_paddedStride = maxAlignment * (uint)glm::ceil(m_alignedStride / (float)maxAlignment);
    }
    
    const VertexElement* VertexLayout::TryGetElement(uint32_t nameHashId, uint32_t* index) const
    {
        auto iterator = m_elementMap.find(nameHashId);
        
        if (iterator != m_elementMap.end())
        {
            *index = iterator->second;
            return (m_elements.data() + iterator->second);
        }

        return nullptr;
    }

    void VertexLayout::FillElementMap()
    {
        auto* elements = m_elements.data();

        for (auto i = 0u; i < m_elements.size(); ++i)
        {
            m_elementMap[elements[i].NameHashId] = i;
        }
    }

    const ResourceElement* ResourceLayout::TryGetElement(uint32_t nameHashId, uint32_t* index) const
    {
        auto iterator = m_elementMap.find(nameHashId);

        if (iterator != m_elementMap.end())
        {
            *index = iterator->second;
            return (m_elements.data() + iterator->second);
        }

        return nullptr;
    }

    void ResourceLayout::FillElementMap()
    {
        auto* elements = m_elements.data();

        for (auto i = 0u; i < m_elements.size(); ++i)
        {
            m_elementMap[elements[i].NameHashId] = i;
        }
    }
}

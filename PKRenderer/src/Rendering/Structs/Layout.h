#pragma once
#include "PrecompiledHeader.h"
#include "Utilities/StringHashID.h"
#include "Math/PKMath.h"
#include "Rendering/Structs/Enums.h"

namespace PK::Rendering::Structs
{
    using namespace PK::Math;
    using namespace PK::Utilities;

    struct IndexRange
    {
        uint offset;
        uint count;
    };

    struct ResourceElement
    {
        uint32_t NameHashId = 0;
        ResourceType Type = ResourceType::Invalid;
        uint8_t Binding = 0;
        uint16_t Count = 0;
    
        ResourceElement() = default;

        ResourceElement(ResourceType type, const std::string& name, uint8_t binding, uint16_t count) : NameHashId(StringHashID::StringToID(name)), Type(type), Binding(binding), Count(count)
        {
        }
    };

    class ResourceLayout
    {
        public:
            ResourceLayout() {}

            ResourceLayout(std::initializer_list<ResourceElement> elements) : m_elements(elements)
            {
                FillElementMap();
            }

            ResourceLayout(std::vector<ResourceElement> elements) : m_elements(elements)
            {
                FillElementMap();
            }

            const ResourceElement* TryGetElement(uint32_t nameHashId, uint32_t* index) const;

            std::vector<ResourceElement>::iterator begin() { return m_elements.begin(); }
            std::vector<ResourceElement>::iterator end() { return m_elements.end(); }
            std::vector<ResourceElement>::const_iterator begin() const { return m_elements.begin(); }
            std::vector<ResourceElement>::const_iterator end() const { return m_elements.end(); }

        private:
            void FillElementMap();

            std::vector<ResourceElement> m_elements;
            std::unordered_map<uint32_t, uint32_t> m_elementMap;
    };

    struct VertexElement
    {
        uint32_t NameHashId = 0;
        ElementType Type = ElementType::Invalid;
        ushort Location = 0;

        VertexElement() = default;

        VertexElement(ElementType type, const std::string& name, ushort location) : NameHashId(StringHashID::StringToID(name)), Type(type), Location(location)
        {
        }
    };

    class VertexLayout
    {
        public:
            VertexLayout() {}

            VertexLayout(std::initializer_list<VertexElement> elements) : m_elements(elements)
            {
                FillElementMap();
            }

            VertexLayout(std::vector<VertexElement> elements) : m_elements(elements)
            {
                FillElementMap();
            }

            const VertexElement* TryGetElement(uint32_t nameHashId, uint32_t* index) const;

            std::vector<VertexElement>::iterator begin() { return m_elements.begin(); }
            std::vector<VertexElement>::iterator end() { return m_elements.end(); }
            std::vector<VertexElement>::const_iterator begin() const { return m_elements.begin(); }
            std::vector<VertexElement>::const_iterator end() const { return m_elements.end(); }

        private:
            void FillElementMap();

            std::vector<VertexElement> m_elements;
            std::unordered_map<uint32_t, uint32_t> m_elementMap;
    };

    struct BufferElement
    {
        uint32_t NameHashId = 0;
        ElementType Type = ElementType::Invalid;
        ushort Size = 0;
        ushort Offset = 0;
        ushort AlignedOffset = 0;
        bool Normalized = false;
    
        BufferElement() = default;
    
        BufferElement(ElementType type, const std::string& name, ushort count = 1, bool normalized = false) : NameHashId(StringHashID::StringToID(name)), Type(type), Size(ElementConvert::Size(type)* count), Offset(0), AlignedOffset(0), Normalized(normalized)
        {
        }

        BufferElement(ElementType type, uint32_t nameHashId, ushort count = 1, bool normalized = false) : NameHashId(nameHashId), Type(type), Size(ElementConvert::Size(type) * count), Offset(0), AlignedOffset(0), Normalized(normalized)
        {
        }
    };
    
    class BufferLayout
    {
        public:
            BufferLayout() {}
    
            BufferLayout(std::initializer_list<BufferElement> elements) : m_elements(elements)
            {
                CalculateOffsetsAndStride();
            }
    
            BufferLayout(std::vector<BufferElement> elements) : m_elements(elements)
            {
                CalculateOffsetsAndStride();
            }
        
            inline uint GetStride(BufferUsage usage) const { return ((uint)usage & ((uint)BufferUsage::Storage | (uint)BufferUsage::Uniform)) != 0 ? m_alignedStride : m_stride; }
            constexpr inline uint GetStride() const { return m_stride; }
            constexpr inline uint GetAlignedStride() const { return m_alignedStride; }
            constexpr inline uint GetPaddedStride() const { return m_alignedStride; }
            inline const std::vector<BufferElement>& GetElements() const { return m_elements; }
        
            std::vector<BufferElement>::iterator begin() { return m_elements.begin(); }
            std::vector<BufferElement>::iterator end() { return m_elements.end(); }
            std::vector<BufferElement>::const_iterator begin() const { return m_elements.begin(); }
            std::vector<BufferElement>::const_iterator end() const { return m_elements.end(); }
    
        private:
            void CalculateOffsetsAndStride();
            
            std::vector<BufferElement> m_elements;
            uint m_stride = 0;
            uint m_alignedStride = 0;
            uint m_paddedStride = 0;
    };
}
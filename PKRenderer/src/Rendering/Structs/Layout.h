#pragma once
#include "PrecompiledHeader.h"
#include "Core/Services/StringHashID.h"
#include "Math/Types.h"
#include "Rendering/Structs/Enums.h"

namespace PK::Rendering::Structs
{
    struct ConstantVariable
    {
        uint32_t NameHashId = 0;
        uint16_t Size;
        uint16_t Offset;
        uint32_t StageFlags;

        ConstantVariable() = default;

        ConstantVariable(const std::string & name, uint16_t size, uint16_t offset, uint32_t stageFlags) : 
            NameHashId(Core::Services::StringHashID::StringToID(name)), 
            Size(size),
            Offset(offset),
            StageFlags(stageFlags)
        {
        }
    };

    class ConstantBufferLayout : public std::map<uint32_t, ConstantVariable>
    {
        public:
            ConstantBufferLayout() {}

            ConstantBufferLayout(std::initializer_list<ConstantVariable> elements)
            {
                FillElementMap(elements.begin(), elements.size());
            }

            ConstantBufferLayout(std::vector<ConstantVariable> elements)
            {
                FillElementMap(elements.data(), elements.size());
            }

            const ConstantVariable* TryGetElement(uint32_t nameHashId) const;

        private:
            void FillElementMap(const ConstantVariable* variables, size_t count);
    };


    struct ResourceElement
    {
        uint32_t NameHashId = 0;
        ResourceType Type = ResourceType::Invalid;
        uint16_t Count = 0;
    
        ResourceElement() = default;

        ResourceElement(ResourceType type, const std::string& name, uint16_t count) :
            NameHashId(Core::Services::StringHashID::StringToID(name)),
            Type(type), 
            Count(count)
        {
        }
    };

    class ResourceLayout : public std::vector<ResourceElement>
    {
        public:
            ResourceLayout() {}

            ResourceLayout(std::initializer_list<ResourceElement> elements) : std::vector<ResourceElement>(elements)
            {
                FillElementMap();
            }

            ResourceLayout(std::vector<ResourceElement> elements) : std::vector<ResourceElement>(elements)
            {
                FillElementMap();
            }

            const ResourceElement* TryGetElement(uint32_t nameHashId, uint32_t* index) const;

        private:
            void FillElementMap();
            std::unordered_map<uint32_t, uint32_t> m_elementMap;
    };


    struct BufferElement
    {
        uint32_t NameHashId = 0;
        ElementType Type = ElementType::Invalid;
        byte Count = 1;
        byte Location = 0;

        uint16_t Offset = 0;
        uint16_t AlignedOffset = 0;
        uint16_t Size() const { return ElementConvert::Size(Type) * Count; }

        BufferElement() = default;

        BufferElement(ElementType type, const std::string& name, byte count = 1, byte location = 0) : 
            NameHashId(Core::Services::StringHashID::StringToID(name)),
            Type(type), 
            Count(count), 
            Offset(0), 
            AlignedOffset(0), 
            Location(location)
        {
        }

        BufferElement(ElementType type, uint32_t nameHashId, byte count = 1, byte location = 0) :
            NameHashId(nameHashId), 
            Type(type), 
            Count(count), 
            Offset(0), 
            AlignedOffset(0), 
            Location(location)
        {
        }
    };

    constexpr static bool operator == (const BufferElement& a, const BufferElement& b)
    {
        return a.NameHashId == b.NameHashId &&
               a.Type == b.Type &&
               a.Count == b.Count &&
               a.Location == b.Location &&
               a.Offset == b.Offset &&
               a.AlignedOffset == b.AlignedOffset;
    }

    constexpr static bool operator != (const BufferElement& a, const BufferElement& b)
    {
        return !(a == b);
    }
    
    class BufferLayout : public std::vector<BufferElement>
    {
        public:

            BufferLayout() {}
    
            BufferLayout(BufferElement* elements, size_t count) : std::vector<BufferElement>(elements, elements + count)
            {
                CalculateOffsetsAndStride();
            }

            BufferLayout(std::initializer_list<BufferElement> elements) : std::vector<BufferElement>(elements)
            {
                CalculateOffsetsAndStride();
            }
    
            BufferLayout(std::vector<BufferElement> elements) : std::vector<BufferElement>(elements)
            {
                CalculateOffsetsAndStride();
            }
        
            inline uint32_t GetStride(BufferUsage usage) const { return (usage & BufferUsage::AlignedTypes) != 0 ? m_alignedStride : m_stride; }
            constexpr inline uint32_t GetStride() const { return m_stride; }
            constexpr inline uint32_t GetAlignedStride() const { return m_alignedStride; }
            constexpr inline uint32_t GetPaddedStride() const { return m_paddedStride; }
        
            const BufferElement* TryGetElement(uint32_t nameHashId, uint32_t* index) const;
    
        private:
            void CalculateOffsetsAndStride();

            std::map<uint32_t, uint32_t> m_elementMap;
            uint32_t m_stride = 0;
            uint32_t m_alignedStride = 0;
            uint32_t m_paddedStride = 0;
    };
}
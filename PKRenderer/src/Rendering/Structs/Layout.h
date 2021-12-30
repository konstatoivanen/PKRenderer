#pragma once
#include "PrecompiledHeader.h"
#include "Core/Services/StringHashID.h"
#include "Math/Types.h"
#include "Rendering/Structs/Enums.h"

namespace PK::Rendering::Structs
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core::Services;

    struct ConstantVariable
    {
        uint32_t NameHashId = 0;
        uint16_t Size;
        uint16_t Offset;
        uint32_t StageFlags;

        ConstantVariable() = default;

        ConstantVariable(const std::string & name, uint16_t size, uint16_t offset, uint32_t stageFlags) : 
            NameHashId(StringHashID::StringToID(name)), 
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
            NameHashId(StringHashID::StringToID(name)), 
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

        ushort Offset = 0;
        ushort AlignedOffset = 0;
    
        ushort Size() const { return ElementConvert::Size(Type) * Count; }

        BufferElement() = default;

        BufferElement(ElementType type, const std::string& name, byte count = 1, byte location = 0) : 
            NameHashId(StringHashID::StringToID(name)), 
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
    
    class BufferLayout : public std::vector<BufferElement>
    {
        public:
            BufferLayout() {}
    
            BufferLayout(std::initializer_list<BufferElement> elements) : std::vector<BufferElement>(elements)
            {
                CalculateOffsetsAndStride();
            }
    
            BufferLayout(std::vector<BufferElement> elements) : std::vector<BufferElement>(elements)
            {
                CalculateOffsetsAndStride();
            }
        
            inline uint GetStride(BufferUsage usage) const { return ((uint)usage & ((uint)BufferUsage::Storage | (uint)BufferUsage::Constant)) != 0 ? m_alignedStride : m_stride; }
            constexpr inline uint GetStride() const { return m_stride; }
            constexpr inline uint GetAlignedStride() const { return m_alignedStride; }
            constexpr inline uint GetPaddedStride() const { return m_paddedStride; }
        
            const BufferElement* TryGetElement(uint32_t nameHashId, uint32_t* index) const;
    
        private:
            void CalculateOffsetsAndStride();
            
            std::map<uint32_t, uint32_t> m_elementMap;
            uint m_stride = 0;
            uint m_alignedStride = 0;
            uint m_paddedStride = 0;
    };
}
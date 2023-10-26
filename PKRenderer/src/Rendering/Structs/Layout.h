#pragma once
#include "PrecompiledHeader.h"
#include "Core/Services/StringHashID.h"
#include "Math/Types.h"
#include "Utilities/FixedList.h"
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
        uint8_t WriteStageMask = 0u;
        uint16_t Count = 0;
    
        ResourceElement() = default;

        ResourceElement(ResourceType type, const std::string& name, uint8_t writeStageMask, uint16_t count) :
            NameHashId(Core::Services::StringHashID::StringToID(name)),
            Type(type),
            WriteStageMask(writeStageMask),
            Count(count)
        {
        }
    };

    struct ResourceLayout : public PK::Utilities::FixedList<ResourceElement, PK_MAX_DESCRIPTORS_PER_SET>
    {
        ResourceLayout() {}
        ResourceLayout(std::initializer_list<ResourceElement> elements) : PK::Utilities::FixedList<ResourceElement, PK_MAX_DESCRIPTORS_PER_SET>(elements) {}
        ResourceLayout(std::vector<ResourceElement> elements) : PK::Utilities::FixedList<ResourceElement, PK_MAX_DESCRIPTORS_PER_SET>(elements.data(), elements.size()) {}
        const ResourceElement* TryGetElement(uint32_t nameHashId, uint32_t* index) const;
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

        BufferElement(ElementType type, const std::string& name, byte count = 1, byte location = 0, uint16_t offset = 0, uint16_t alignedOffset = 0) : 
            NameHashId(Core::Services::StringHashID::StringToID(name)),
            Type(type), 
            Count(count), 
            Offset(offset),
            AlignedOffset(alignedOffset),
            Location(location)
        {
        }

        BufferElement(ElementType type, uint32_t nameHashId, byte count = 1, byte location = 0, uint16_t offset = 0, uint16_t alignedOffset = 0) :
            NameHashId(nameHashId), 
            Type(type), 
            Count(count), 
            Offset(offset),
            AlignedOffset(alignedOffset),
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
    
            BufferLayout(BufferElement* elements, size_t count, bool applyOffsets = true) : std::vector<BufferElement>(elements, elements + count)
            {
                CalculateOffsetsAndStride(applyOffsets);
            }

            BufferLayout(std::initializer_list<BufferElement> elements, bool applyOffsets = true) : std::vector<BufferElement>(elements)
            {
                CalculateOffsetsAndStride(applyOffsets);
            }
    
            BufferLayout(std::vector<BufferElement> elements, bool applyOffsets = true) : std::vector<BufferElement>(elements)
            {
                CalculateOffsetsAndStride(applyOffsets);
            }
        
            inline uint32_t GetStride(BufferUsage usage) const { return (usage & BufferUsage::AlignedTypes) != 0 ? m_alignedStride : m_stride; }
            constexpr inline uint32_t GetStride() const { return m_stride; }
            constexpr inline uint32_t GetAlignedStride() const { return m_alignedStride; }
            constexpr inline uint32_t GetPaddedStride() const { return m_paddedStride; }
            const BufferElement* TryGetElement(uint32_t nameHashId, uint32_t* index) const;
            void CalculateOffsetsAndStride(bool applyOffsets);

        private:
            std::map<uint32_t, uint32_t> m_elementMap;
            uint32_t m_stride = 0;
            uint32_t m_alignedStride = 0;
            uint32_t m_paddedStride = 0;
    };

    struct ShaderBindingTableInfo
    {
        enum { HandleMaxSize = 64 };
        enum { MaxHandles = 8 };

        uint8_t handleData[HandleMaxSize * MaxHandles];
        uint16_t byteOffsets[(uint32_t)RayTracingShaderGroup::MaxCount];
        uint16_t byteStrides[(uint32_t)RayTracingShaderGroup::MaxCount];
        uint8_t offsets[(uint32_t)RayTracingShaderGroup::MaxCount];
        uint8_t counts[(uint32_t)RayTracingShaderGroup::MaxCount];
        const ResourceLayout* layouts[(uint32_t)RayTracingShaderGroup::MaxCount];
        uint16_t handleSize;
        uint16_t handleSizeAligned;
        uint16_t tableAlignment;
        uint16_t totalTableSize;
        uint16_t totalHandleCount;
    };
}
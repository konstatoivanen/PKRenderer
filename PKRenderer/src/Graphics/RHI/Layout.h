#pragma once
#include <vector>
#include "Math/Types.h"
#include "Utilities/FixedList.h"
#include "Utilities/FastSet.h"
#include "Utilities/NameID.h"
#include "Graphics/RHI/Structs.h"

namespace PK::Graphics::RHI
{
    struct PushConstant
    {
        Utilities::NameID name = 0u;
        ShaderStageFlags stageFlags;
        uint16_t size;
        uint16_t offset;

        PushConstant() = default;

        PushConstant(Utilities::NameID name, uint16_t size, uint16_t offset, ShaderStageFlags stageFlags) :
            name(name),
            stageFlags(stageFlags),
            size(size),
            offset(offset)
        {
        }
    };

    struct PushConstantLayout : public PK::Utilities::InlineList<PushConstant, PK_MAX_PUSH_CONSTANTS>
    {
        PushConstantLayout() {}
        PushConstantLayout(const PushConstant* variables, size_t count) : InlineList(variables, count) {}
    };


    struct ResourceElement
    {
        Utilities::NameID name = 0u;
        ShaderStageFlags writeStageMask = ShaderStageFlags::None;
        uint16_t count = 0;
        ResourceType type = ResourceType::Invalid;

        ResourceElement() = default;

        ResourceElement(ResourceType type, Utilities::NameID name, ShaderStageFlags writeStageMask, uint16_t count) :
            name(name),
            writeStageMask(writeStageMask),
            count(count),
            type(type)
        {
        }
    };

    struct ResourceLayout : public PK::Utilities::InlineList<ResourceElement, PK_MAX_DESCRIPTORS_PER_SET>
    {
        ResourceLayout() {}
        ResourceLayout(std::initializer_list<ResourceElement> elements) : InlineList(elements) {}
        ResourceLayout(std::vector<ResourceElement> elements) : InlineList(elements.data(), elements.size()) {}
        const ResourceElement* TryGetElement(Utilities::NameID name, uint32_t* index) const;
    };


    struct BufferElement
    {
        Utilities::NameID name = 0u;
        ElementType type = ElementType::Invalid;
        uint8_t count = 1;
        uint8_t location = 0;
        uint16_t offset = 0;
        uint16_t alignedOffset = 0;

        uint16_t GetSize() const { return ElementConvert::Size(type) * count; }

        BufferElement() = default;

        BufferElement(ElementType type, Utilities::NameID name, uint8_t count = 1, uint8_t location = 0, uint16_t offset = 0, uint16_t alignedOffset = 0) :
            name(name),
            type(type), 
            count(count), 
            location(location),
            offset(offset),
            alignedOffset(alignedOffset)
        {
        }

        constexpr bool operator== (const BufferElement& b)
        {
            return name == b.name && type == b.type && count == b.count && location == b.location && offset == b.offset && alignedOffset == b.alignedOffset;
        }
    };

    struct BufferElementNameHash
    {
        size_t operator()(const BufferElement& k) const noexcept
        {
            return (size_t)k.name.identifier;
        }
    };

    struct BufferLayout : public std::vector<BufferElement>
    {
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
        constexpr inline uint64_t GetHash() const { return m_hash; }
        constexpr inline bool CompareFast(const BufferLayout& other) const { return m_stride == other.m_stride && m_hash == other.m_hash; }
        void CalculateOffsetsAndStride(bool applyOffsets);

    private:
        uint64_t m_hash = 0ull;
        uint32_t m_stride = 0;
        uint32_t m_alignedStride = 0;
        uint32_t m_paddedStride = 0;
    };


    struct VertexInputLayout : public Utilities::FastSet<BufferElement, BufferElementNameHash>
    {
        VertexInputLayout() : FastSet() {}
        const BufferElement* TryGetElement(Utilities::NameID name, uint32_t* index) const;
    };


    struct VertexStreamElement
    {
        Utilities::NameID name = 0u;
        uint8_t stream = 0u;
        InputRate inputRate = InputRate::PerVertex;
        uint16_t stride = 0u;
        uint16_t offset = 0u;
        uint16_t size = 0u;
        
        VertexStreamElement() = default;

        VertexStreamElement(uint16_t size, Utilities::NameID name, uint8_t stream = 0u, InputRate inputRate = InputRate::PerVertex) :
            name(name),
            stream(stream),
            inputRate(inputRate),
            stride(0u),
            offset(0u),
            size(size)
        {
        }

        VertexStreamElement(ElementType type, Utilities::NameID name, uint8_t stream = 0u, InputRate inputRate = InputRate::PerVertex) :
            name(name),
            stream(stream),
            inputRate(inputRate),
            stride(0u),
            offset(0u),
            size(ElementConvert::Size(type))
        {
        }
    };

    struct VertexStreamLayout : public PK::Utilities::InlineList<VertexStreamElement, PK_MAX_VERTEX_ATTRIBUTES>
    {
        using PK::Utilities::InlineList<VertexStreamElement, PK_MAX_VERTEX_ATTRIBUTES>::Add;

        VertexStreamLayout() {}

        VertexStreamLayout(const VertexStreamElement* elements, size_t count) : InlineList(elements, count)
        {
            CalculateOffsetsAndStride();
        }

        VertexStreamLayout(std::initializer_list<VertexStreamElement> elements) : InlineList(elements)
        {
            CalculateOffsetsAndStride();
        }

        VertexStreamLayout(std::vector<VertexStreamElement> elements) : InlineList(elements.data(), elements.size())
        {
            CalculateOffsetsAndStride();
        }

        inline uint16_t GetStride(uint32_t stream) const { return m_streamStrides[stream]; }
        inline uint16_t GetStride() const { return m_totalStride; }
        
        void Add(const VertexStreamLayout& other);
        void Add(const VertexStreamLayout& other, uint32_t stream);

        void CalculateOffsetsAndStride();

    private:
        uint16_t m_streamStrides[PK_MAX_VERTEX_ATTRIBUTES]{};
        uint16_t m_totalStride = 0u;
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
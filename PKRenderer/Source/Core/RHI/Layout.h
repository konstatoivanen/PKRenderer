#pragma once
#include <vector>
#include "Core/Utilities/FixedList.h"
#include "Core/Utilities/FastMap.h"
#include "Core/Utilities/NameID.h"
#include "Core/RHI/Structs.h"

namespace PK
{
    struct BufferElement
    {
        NameID name = 0u;
        ElementType format = ElementType::Invalid;
        uint8_t count = 1;
        uint8_t location = 0;
        uint16_t offset = 0;
        uint16_t alignedOffset = 0;

        uint16_t GetSize() const { return (uint16_t)RHIEnumConvert::Size(format) * count; }

        BufferElement() = default;

        BufferElement(ElementType format, NameID name, uint8_t count = 1, uint8_t location = 0, uint16_t offset = 0, uint16_t alignedOffset = 0) :
            name(name),
            format(format),
            count(count), 
            location(location),
            offset(offset),
            alignedOffset(alignedOffset)
        {
        }

        constexpr bool operator== (const BufferElement& b)
        {
            return name == b.name && format == b.format && count == b.count && location == b.location && offset == b.offset && alignedOffset == b.alignedOffset;
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


    struct ShaderPushConstant
    {
        NameID name = 0u;
        uint16_t offset;
        uint16_t size;

        ShaderPushConstant() = default;

        ShaderPushConstant(NameID name, uint16_t size, uint16_t offset) :
            name(name),
            offset(offset),
            size(size)
        {
        }
    };

    struct ShaderPushConstantLayout : public FixedList<ShaderPushConstant, PK_RHI_MAX_PUSH_CONSTANTS>
    {
        ShaderPushConstantLayout() {}
        ShaderPushConstantLayout(const ShaderPushConstant* variables, size_t count) : FixedList(variables, count) {}
    };


    struct ShaderResourceElement
    {
        NameID name = 0u;
        ShaderStageFlags writeStageMask = ShaderStageFlags::None;
        uint16_t count = 0;
        ShaderResourceType type = ShaderResourceType::Invalid;

        ShaderResourceElement() = default;

        ShaderResourceElement(ShaderResourceType type, NameID name, ShaderStageFlags writeStageMask, uint16_t count) :
            name(name),
            writeStageMask(writeStageMask),
            count(count),
            type(type)
        {
        }
    };

    struct ShaderResourceLayout : public FixedList<ShaderResourceElement, PK_RHI_MAX_DESCRIPTORS_PER_SET>
    {
        ShaderResourceLayout() {}
        ShaderResourceLayout(std::initializer_list<ShaderResourceElement> elements) : FixedList(elements) {}
        ShaderResourceLayout(std::vector<ShaderResourceElement> elements) : FixedList(elements.data(), elements.size()) {}
        const ShaderResourceElement* TryGetElement(NameID name, uint32_t* index) const;
    };


    struct ShaderVertexInputElement
    {
        NameID name = 0u;
        ElementType format = ElementType::Invalid;
        uint16_t location = 0;

        ShaderVertexInputElement() = default;

        ShaderVertexInputElement(NameID name, ElementType format, uint16_t location) : name(name), format(format), location(location)
        {
        }

        constexpr bool operator== (const ShaderVertexInputElement& b)
        {
            return name == b.name && format == b.format && location == b.location;
        }
    };

    struct ShaderVertexInputElementHash
    {
        size_t operator()(const ShaderVertexInputElement& k) const noexcept
        {
            return (size_t)k.name.identifier;
        }
    };

    struct ShaderVertexInputLayout : public FixedSet8<ShaderVertexInputElement, PK_RHI_MAX_VERTEX_ATTRIBUTES, ShaderVertexInputElementHash>
    {
        ShaderVertexInputLayout() : IFastSet() {}
        const ShaderVertexInputElement* TryGetElement(NameID name, uint32_t* index) const;
    };


    struct VertexStreamElement
    {
        NameID name = 0u;
        uint8_t stream = 0u;
        InputRate inputRate = InputRate::PerVertex;
        uint16_t stride = 0u;
        uint16_t offset = 0u;
        uint16_t size = 0u;
        
        VertexStreamElement() = default;

        VertexStreamElement(uint16_t size, NameID name, uint8_t stream = 0u, InputRate inputRate = InputRate::PerVertex) :
            name(name),
            stream(stream),
            inputRate(inputRate),
            stride(0u),
            offset(0u),
            size(size)
        {
        }

        VertexStreamElement(ElementType format, NameID name, uint8_t stream = 0u, InputRate inputRate = InputRate::PerVertex) :
            name(name),
            stream(stream),
            inputRate(inputRate),
            stride(0u),
            offset(0u),
            size((uint16_t)RHIEnumConvert::Size(format))
        {
        }
    };

    struct VertexStreamLayout : public FixedList<VertexStreamElement, PK_RHI_MAX_VERTEX_ATTRIBUTES>
    {
        using FixedList<VertexStreamElement, PK_RHI_MAX_VERTEX_ATTRIBUTES>::Add;

        VertexStreamLayout() {}

        VertexStreamLayout(const VertexStreamElement* elements, size_t count) : FixedList(elements, count)
        {
            CalculateOffsetsAndStride();
        }

        VertexStreamLayout(std::initializer_list<VertexStreamElement> elements) : FixedList(elements)
        {
            CalculateOffsetsAndStride();
        }

        VertexStreamLayout(std::vector<VertexStreamElement> elements) : FixedList(elements.data(), elements.size())
        {
            CalculateOffsetsAndStride();
        }

        inline uint16_t GetStride(uint32_t stream) const { return m_streamStrides[stream]; }
        inline uint16_t GetStride() const { return m_totalStride; }
        
        void Add(const VertexStreamLayout& other);
        void Add(const VertexStreamLayout& other, uint32_t stream);

        void CalculateOffsetsAndStride();

    private:
        uint16_t m_streamStrides[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
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
        const ShaderResourceLayout* layouts[(uint32_t)RayTracingShaderGroup::MaxCount];
        uint16_t handleSize;
        uint16_t handleSizeAligned;
        uint16_t tableAlignment;
        uint16_t totalTableSize;
        uint16_t totalHandleCount;
    };
}

#pragma once
#include "Core/Base/Containers/ArrayList.h"
#include "Core/Base/Containers/HashMap.h"
#include "Core/Base/Types/NameID.h"
#include "Core/RHI/Structs.h"

namespace PK
{
    struct ShaderParameter
    {
        NameID name = 0u;
        ShaderStageFlags accessMask = ShaderStageFlags::None;
        ShaderStageFlags writeMask = ShaderStageFlags::None;
        Uniform type = Uniform::Invalid;
        uint8_t offset = 0u;
        uint16_t count = 0u;

        ShaderParameter() = default;

        ShaderParameter(NameID name, ShaderStageFlags accessMask, ShaderStageFlags writeMask, Uniform type, uint8_t offset, uint16_t count) :
            name(name),
            accessMask(accessMask),
            writeMask(writeMask),
            type(type),
            offset(offset),
            count(count)
        {
        }
    };

    struct ShaderParameterLayout : public FixedList<ShaderParameter, PK_RHI_MAX_SHADER_PARAMETERS>
    {
        size_t sizeBytes;

        using TBase = FixedList<ShaderParameter, PK_RHI_MAX_SHADER_PARAMETERS>;
        ShaderParameterLayout() {}
        ShaderParameterLayout(const ShaderParameter* elements, size_t count) : TBase(elements, count) {}
        ShaderParameterLayout(initializer_list<ShaderParameter> elements) : TBase(elements) {}
        const ShaderParameter* TryGetElement(NameID name, uint32_t* index) const;
    };

    struct ShaderVertexInputElement
    {
        NameID name = 0u;
        Uniform format = Uniform::Invalid;
        uint8_t location = 0;

        ShaderVertexInputElement() = default;

        ShaderVertexInputElement(NameID name, Uniform format, uint8_t location) : name(name), format(format), location(location)
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
            return k.name.identifier;
        }
    };

    struct ShaderVertexInputLayout : public FixedSet8<ShaderVertexInputElement, PK_RHI_MAX_VERTEX_ATTRIBUTES, ShaderVertexInputElementHash>
    {
        ShaderVertexInputLayout() : IHashSet() {}
        const ShaderVertexInputElement* TryGetElement(NameID name, uint32_t* index) const;
    };


    struct VertexStreamElement
    {
        NameID name = 0u;
        Uniform format = Uniform::Invalid;
        uint8_t stream = 0u;
        uint16_t stride = 0u;
        uint16_t offset = 0u;
        
        VertexStreamElement() = default;

        VertexStreamElement(Uniform format, NameID name, uint8_t stream = 0u) :
            name(name),
            format(format),
            stream(stream),
            stride(0u),
            offset(0u)
        {
        }
    };

    struct VertexStreamLayout : public FixedList<VertexStreamElement, PK_RHI_MAX_VERTEX_ATTRIBUTES>
    {
        using TBase = FixedList<VertexStreamElement, PK_RHI_MAX_VERTEX_ATTRIBUTES>;
        using TBase::Add;

        VertexStreamLayout() {}

        VertexStreamLayout(const VertexStreamElement* elements, size_t count) : TBase(elements, count)
        {
            CalculateOffsetsAndStride();
        }

        VertexStreamLayout(initializer_list<VertexStreamElement> elements) : TBase(elements)
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
        constexpr static const uint32_t HandleMaxSize = 64u;
        constexpr static const uint32_t MaxHandles = 8u;

        uint8_t handleData[HandleMaxSize * MaxHandles];
        uint16_t byteOffsets[(uint32_t)RayTracingShaderGroup::EnumCount];
        uint16_t byteStrides[(uint32_t)RayTracingShaderGroup::EnumCount];
        uint8_t offsets[(uint32_t)RayTracingShaderGroup::EnumCount];
        uint8_t counts[(uint32_t)RayTracingShaderGroup::EnumCount];
        uint16_t handleSize;
        uint16_t handleSizeAligned;
        uint16_t tableAlignment;
        uint16_t totalTableSize;
        uint16_t totalHandleCount;
    };
}

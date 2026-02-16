#pragma once
#include "Core/Utilities/FastBuffer.h"
#include "Core/Utilities/FastMap.h"
#include "Core/RHI/Structs.h"

namespace PK
{
    struct ShaderProperty
    {
        NameID name = 0u;
        ElementType format = ElementType::Invalid;
        uint16_t count = 1u;
        uint16_t offset = 0u;
        uint16_t offsetHandle = 0u;

        uint16_t GetSize() const { return (uint16_t)RHIEnumConvert::Size(format) * count; }

        ShaderProperty() = default;

        ShaderProperty(ElementType format, NameID name, uint8_t count = 1) :
            name(name),
            format(format),
            count(count)
        {
        }

        constexpr bool operator== (const ShaderProperty& b)
        {
            return name == b.name && format == b.format && count == b.count && offset == b.offset;
        }
    };

    struct ShaderPropertyHash
    {
        size_t operator()(const ShaderProperty& k) const noexcept
        {
            return (size_t)k.name.identifier;
        }
    };

    struct ShaderPropertyLayout : public FastSet16<ShaderProperty, ShaderPropertyHash>
    {
        using TBase = FastSet16<ShaderProperty, ShaderPropertyHash>;

        ShaderPropertyLayout() {}
        ShaderPropertyLayout(ShaderProperty* elements, size_t count);
        ShaderPropertyLayout(std::initializer_list<ShaderProperty> elements);

        constexpr inline uint32_t GetStride() const { return m_stride; }
        constexpr inline uint32_t GetStridePadded() const { return m_stridePadded; }
        constexpr inline uint32_t GetStrideMaterial() const { return m_strideMaterial; }
        constexpr inline uint64_t GetHash() const { return m_hash; }
        constexpr inline bool CompareFast(const ShaderPropertyLayout& other) const { return m_stride == other.m_stride && m_hash == other.m_hash; }
        const ShaderProperty* TryGetElement(NameID name) const;
        void CalculateOffsetsAndStride();

    private:
        uint64_t m_hash = 0ull;
        uint16_t m_stride = 0u;
        uint16_t m_stridePadded = 0u;
        uint32_t m_strideMaterial = 0u;
    };

    struct ShaderPropertyWriter
    {
        template<typename T>
        bool Set(const NameID name, const T* src, uint32_t count) { return Write(name, src, sizeof(T) * count); }

        template<typename T>
        bool Set(const NameID name, const T& src) { return Write(name, &src, sizeof(T)); }

        template<typename T>
        bool SetResource(const NameID name, T* value) { return WriteResource(name, value); }

        template<typename T>
        const T* Get(const NameID name) const { return reinterpret_cast<const T*>(Read(name)); }

        template<typename T>
        T* GetResource(const NameID name) const { return *reinterpret_cast<T* const*>(ReadResource(name)); }

    protected:
        void BeginWrite(const ShaderPropertyLayout* layout, void* memory);
        void* EndWrite();

    private:
        bool Write(NameID name, const void* value, size_t size);
        bool WriteResource(NameID name, void* value);
        const void* Read(NameID name) const;
        const void* ReadResource(NameID name) const;

        const ShaderPropertyLayout* m_layout = nullptr;
        uint8_t* m_memory = nullptr;
    };

    struct ShaderPropertyBlock : public NoCopy, public ShaderPropertyWriter
    {
        ShaderPropertyBlock(const ShaderPropertyLayout& layout);
        ShaderPropertyBlock(ShaderProperty* elements, size_t count);
        ShaderPropertyBlock(std::initializer_list<ShaderProperty> elements);
        constexpr const ShaderPropertyLayout& GetLayout() const { return m_layout; }
        constexpr const void* GetData() const { return m_properties.GetData(); }
    private:
        ShaderPropertyLayout m_layout;
        FastBuffer<uint8_t> m_properties;
    };
}

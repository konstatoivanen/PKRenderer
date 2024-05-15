#pragma once
#include "Utilities/Ref.h"
#include <cstdint>

namespace PK::Utilities
{
    struct NameID;
}

namespace PK::Rendering::RHI
{
    struct Driver;
    struct BuiltInResources;
    struct DriverMemoryInfo;
    struct IndexRange;
    struct ImageUploadRange;
    struct DrawIndexedIndirectCommand;
    struct DrawIndirectCommand;
    struct SamplerDescriptor;
    struct TextureDescriptor;
    struct TextureViewRange;
    struct MultisamplingParameters;
    struct DepthStencilParameters;
    struct RasterizationParameters;
    struct BlendParameters;
    struct FixedFunctionShaderAttributes;
    struct FixedFunctionState;
    struct DriverMemoryInfo;
    struct AccelerationStructureGeometryInfo;
    struct WindowProperties;

    struct PushConstant;
    struct PushConstantLayout;
    struct ResourceElement;
    struct ResourceLayout;
    struct BufferElement;
    struct BufferLayout;
    struct VertexStreamElement;
    struct VertexStreamLayout;
    struct ShaderBindingTableInfo;

    enum class APIType;
    enum class QueueType;
    enum class SamplerType : uint8_t;
    enum class TextureBindMode : uint8_t;
    enum class FilterMode : uint8_t;
    enum class PolygonMode : uint8_t;
    enum class Topology : uint8_t;
    enum class WrapMode : uint8_t;
    enum class ColorMask : uint8_t;
    enum class LogicOp : uint8_t;
    enum class FrontFace : uint8_t;
    enum class LoadOp : uint8_t;
    enum class StoreOp : uint8_t;
    enum class BorderColor : uint8_t;
    enum class InputRate : uint8_t;
    enum class BufferUsage : uint32_t;
    enum class TextureUsage : uint16_t;
    enum class TextureFormat : uint16_t;
    enum class RayTracingShaderGroup;

    namespace Objects
    {
        struct AccelerationStructure;
        template <typename>
        struct BindArray;
        struct Buffer;
        struct CommandBuffer;
        struct QueueSet;
        struct Shader;
        struct Texture;
        struct Window;
        template<typename T>
        using BindArrayRef = Utilities::Ref<BindArray<T>>;

        typedef BindArray<Texture> TextureBindArray;
        typedef BindArray<Buffer> BufferBindArray;

        typedef Utilities::Ref<AccelerationStructure> AccelerationStructureRef;
        typedef Utilities::Ref<TextureBindArray> TextureBindArrayRef;
        typedef Utilities::Ref<BufferBindArray> BufferBindArrayRef;
        typedef Utilities::Ref<Buffer> BufferRef;
        typedef Utilities::Ref<Texture> TextureRef;
        typedef Utilities::Scope<Window> WindowScope;
    }

    Driver* RHIGetDriver();
    APIType RHIGetActiveAPI();
    RHI::Objects::QueueSet* RHIGetQueues();
    RHI::Objects::CommandBuffer* RHIGetCommandBuffer(QueueType queue);
    DriverMemoryInfo RHIGetMemoryInfo();
    size_t RHIGetBufferOffsetAlignment(BufferUsage usage);
    const BuiltInResources* RHIGetBuiltInResources();
    void RHIGC();

    RHI::Objects::BufferRef RHICreateBuffer(size_t size, BufferUsage usage, const char* name);
    RHI::Objects::TextureRef RHICreateTexture(const TextureDescriptor& descriptor, const char* name);
    RHI::Objects::AccelerationStructureRef RHICreateAccelerationStructure(const char* name);
    RHI::Objects::WindowScope RHICreateWindow(const WindowProperties& properties);
    
    template<typename T>
    RHI::Objects::BindArrayRef<T> RHICreateBindArray(size_t capacity);

    template<typename T>
    inline RHI::Objects::BufferRef RHICreateBuffer(size_t count, BufferUsage usage, const char* name) { return RHICreateBuffer(sizeof(T) * count, usage, name); }

    bool RHIValidateTexture(RHI::Objects::TextureRef& inoutTexture, const TextureDescriptor& descriptor, const char* name);

    bool RHIValidateBuffer(RHI::Objects::BufferRef& inoutBuffer, size_t size, BufferUsage usage, const char* name);

    template<typename T>
    inline bool RHIValidateBuffer(RHI::Objects::BufferRef& inoutBuffer, size_t count, BufferUsage usage, const char* name) { return RHIValidateBuffer(inoutBuffer, sizeof(T) * count, usage, name); }

    void RHISetBuffer(Utilities::NameID name, RHI::Objects::Buffer* buffer, const IndexRange& range);
    void RHISetBuffer(Utilities::NameID name, RHI::Objects::Buffer* buffer);
    void RHISetTexture(Utilities::NameID name, RHI::Objects::Texture* texture, const TextureViewRange& range);
    void RHISetTexture(Utilities::NameID name, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer);
    void RHISetTexture(Utilities::NameID name, RHI::Objects::Texture* texture);
    void RHISetImage(Utilities::NameID name, RHI::Objects::Texture* texture, const TextureViewRange& range);
    void RHISetImage(Utilities::NameID name, RHI::Objects::Texture* texture, uint16_t level, uint16_t layer);
    void RHISetImage(Utilities::NameID name, RHI::Objects::Texture* texture);
    void RHISetSampler(Utilities::NameID name, const SamplerDescriptor& sampler);
    void RHISetAccelerationStructure(Utilities::NameID name, RHI::Objects::AccelerationStructure* structure);
    void RHISetBufferArray(Utilities::NameID name, RHI::Objects::BindArray<RHI::Objects::Buffer>* bufferArray);
    void RHISetTextureArray(Utilities::NameID name, RHI::Objects::BindArray<RHI::Objects::Texture>* textureArray);
    void RHISetConstant(Utilities::NameID name, const void* data, uint32_t size);
    void RHISetKeyword(Utilities::NameID name, bool value);

    template<typename T>
    void RHISetConstant(Utilities::NameID name, const T& value) { RHISetConstant(name, &value, (uint32_t)sizeof(T)); }
}
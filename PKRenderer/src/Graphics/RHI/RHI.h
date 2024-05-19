#pragma once
#include <cstdint>
#include "Utilities/Ref.h"

namespace PK::Assets
{
    enum class PKShaderStageFlags : uint32_t;
    enum class PKElementType : uint16_t;
}

namespace PK::Assets::Shader
{
    struct PKShaderVariant;
}

namespace PK::Utilities
{
    struct NameID;
    struct FenceRef;
    class PropertyBlock;
}

namespace PK::Graphics::RHI
{
    struct BuiltInResources;
    struct DriverMemoryInfo;
    struct IndexRange;
    struct ImageUploadRange;
    struct DrawIndexedIndirectCommand;
    struct DrawIndirectCommand;
    struct TextureViewRange;
    struct MultisamplingParameters;
    struct DepthStencilParameters;
    struct RasterizationParameters;
    struct BlendParameters;
    struct FixedFunctionShaderAttributes;
    struct FixedFunctionState;
    struct DriverMemoryInfo;
    struct AccelerationStructureGeometryInfo;
    struct SamplerDescriptor;
    struct TextureDescriptor;
    struct WindowDescriptor;

    struct PushConstant;
    struct PushConstantLayout;
    struct ResourceElement;
    struct ResourceLayout;
    struct BufferElement;
    struct BufferLayout;
    struct VertexStreamElement;
    struct VertexStreamLayout;
    struct ShaderBindingTableInfo;
    
    typedef PK::Assets::PKShaderStageFlags ShaderStageFlags;
    typedef PK::Assets::PKElementType ElementType;

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

    // Interface types
    struct RHIDriver;
    struct RHIAccelerationStructure;
    template <typename>
    struct RHIBindArray;
    struct RHIBuffer;
    struct RHICommandBuffer;
    struct RHIQueueSet;
    struct RHIShader;
    struct RHITexture;
    struct RHIWindow;
    template<typename T>
    using RHIBindArrayRef = Utilities::Ref<RHIBindArray<T>>;

    typedef RHIBindArray<RHITexture> RHITextureBindArray;
    typedef RHIBindArray<RHIBuffer> RHIBufferBindArray;

    typedef Utilities::Ref<RHIAccelerationStructure> RHIAccelerationStructureRef;
    typedef Utilities::Ref<RHITextureBindArray> RHITextureBindArrayRef;
    typedef Utilities::Ref<RHIBufferBindArray> RHIBufferBindArrayRef;
    typedef Utilities::Ref<RHIBuffer> RHIBufferRef;
    typedef Utilities::Ref<RHITexture> RHITextureRef;
    typedef Utilities::Scope<RHIShader> RHIShaderScope;
    typedef Utilities::Scope<RHIWindow> RHIWindowScope;
    typedef Utilities::Scope<RHIDriver> RHIDriverScope;

    RHIDriver* RHIGetDriver();
    APIType RHIGetActiveAPI();
    RHIQueueSet* RHIGetQueues();
    RHICommandBuffer* RHIGetCommandBuffer(QueueType queue);
    DriverMemoryInfo RHIGetMemoryInfo();
    size_t RHIGetBufferOffsetAlignment(BufferUsage usage);
    const BuiltInResources* RHIGetBuiltInResources();
    void RHIGC();

    RHIDriverScope RHICreateDriver(const char* workingDirectory, APIType api);
    RHIBufferRef RHICreateBuffer(size_t size, BufferUsage usage, const char* name);
    RHITextureRef RHICreateTexture(const TextureDescriptor& descriptor, const char* name);
    RHIAccelerationStructureRef RHICreateAccelerationStructure(const char* name);
    RHIShaderScope RHICreateShader(void* base, PK::Assets::Shader::PKShaderVariant* pVariant, const char* name);
    RHIWindowScope RHICreateWindow(const WindowDescriptor& descriptor);
    
    template<typename T>
    RHIBindArrayRef<T> RHICreateBindArray(size_t capacity);

    template<typename T>
    inline RHIBufferRef RHICreateBuffer(size_t count, BufferUsage usage, const char* name) { return RHICreateBuffer(sizeof(T) * count, usage, name); }

    bool RHIValidateTexture(RHITextureRef& inoutTexture, const TextureDescriptor& descriptor, const char* name);

    bool RHIValidateBuffer(RHIBufferRef& inoutBuffer, size_t size, BufferUsage usage, const char* name);

    template<typename T>
    inline bool RHIValidateBuffer(RHIBufferRef& inoutBuffer, size_t count, BufferUsage usage, const char* name) { return RHIValidateBuffer(inoutBuffer, sizeof(T) * count, usage, name); }

    void RHISetBuffer(Utilities::NameID name, RHIBuffer* buffer, const IndexRange& range);
    void RHISetBuffer(Utilities::NameID name, RHIBuffer* buffer);
    void RHISetTexture(Utilities::NameID name, RHITexture* texture, const TextureViewRange& range);
    void RHISetTexture(Utilities::NameID name, RHITexture* texture, uint16_t level, uint16_t layer);
    void RHISetTexture(Utilities::NameID name, RHITexture* texture);
    void RHISetImage(Utilities::NameID name, RHITexture* texture, const TextureViewRange& range);
    void RHISetImage(Utilities::NameID name, RHITexture* texture, uint16_t level, uint16_t layer);
    void RHISetImage(Utilities::NameID name, RHITexture* texture);
    void RHISetSampler(Utilities::NameID name, const SamplerDescriptor& sampler);
    void RHISetAccelerationStructure(Utilities::NameID name, RHIAccelerationStructure* structure);
    void RHISetBufferArray(Utilities::NameID name, RHIBindArray<RHIBuffer>* bufferArray);
    void RHISetTextureArray(Utilities::NameID name, RHIBindArray<RHITexture>* textureArray);
    void RHISetConstant(Utilities::NameID name, const void* data, uint32_t size);
    void RHISetKeyword(Utilities::NameID name, bool value);

    template<typename T>
    void RHISetConstant(Utilities::NameID name, const T& value) { RHISetConstant(name, &value, (uint32_t)sizeof(T)); }
}
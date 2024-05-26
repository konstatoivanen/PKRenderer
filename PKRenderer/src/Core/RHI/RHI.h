#pragma once
#include <cstdint>
#include "Core/Utilities/Ref.h"

namespace std
{
    template <class _Fty>
    class function;
}

namespace PK::Assets
{
    enum class PKShaderStageFlags : uint32_t;
    enum class PKElementType : uint16_t;
    
    namespace Shader
    {
        struct PKShaderVariant;
    }
}

namespace PK
{
    struct NameID;
    struct FenceRef;
    class PropertyBlock;

    struct DrawIndexedIndirectCommand;
    struct DrawIndirectCommand;
    struct BuiltInResources;
    struct BufferIndexRange;
    struct TextureUploadRange;
    struct TextureViewRange;
    struct MultisamplingParameters;
    struct DepthStencilParameters;
    struct RasterizationParameters;
    struct BlendParameters;
    struct FixedFunctionShaderAttributes;
    struct FixedFunctionState;
    struct RHIDriverMemoryInfo;
    struct AccelerationStructureGeometryInfo;
    struct SamplerDescriptor;
    struct TextureDescriptor;
    struct WindowDescriptor;

    struct BufferElement;
    struct BufferLayout;
    struct ShaderPushConstant;
    struct ShaderPushConstantLayout;
    struct ShaderResourceElement;
    struct ShaderResourceLayout;
    struct ShaderVertexInputLayout;
    struct VertexStreamElement;
    struct VertexStreamLayout;
    struct ShaderBindingTableInfo;
    
    typedef PK::Assets::PKShaderStageFlags ShaderStageFlags;
    typedef PK::Assets::PKElementType ElementType;

    enum class RHIAPI;
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
    using RHIBindArrayRef = Ref<RHIBindArray<T>>;

    typedef RHIBindArray<RHITexture> RHITextureBindArray;
    typedef RHIBindArray<RHIBuffer> RHIBufferBindArray;

    typedef Ref<RHIAccelerationStructure> RHIAccelerationStructureRef;
    typedef Ref<RHITextureBindArray> RHITextureBindArrayRef;
    typedef Ref<RHIBufferBindArray> RHIBufferBindArrayRef;
    typedef Ref<RHIBuffer> RHIBufferRef;
    typedef Ref<RHITexture> RHITextureRef;
    typedef Scope<RHIShader> RHIShaderScope;
    typedef Scope<RHIWindow> RHIWindowScope;
    typedef Scope<RHIDriver> RHIDriverScope;

    // Interface
    namespace RHI
    {
        RHIDriver* GetDriver();
        RHIAPI GetActiveAPI();
        RHIQueueSet* GetQueues();
        RHICommandBuffer* GetCommandBuffer(QueueType queue);
        RHIDriverMemoryInfo GetMemoryInfo();
        size_t GetBufferOffsetAlignment(BufferUsage usage);
        const BuiltInResources* GetBuiltInResources();
        void GC();

        RHIDriverScope CreateDriver(const char* workingDirectory, RHIAPI api);
        RHIBufferRef CreateBuffer(size_t size, BufferUsage usage, const char* name);
        RHITextureRef CreateTexture(const TextureDescriptor& descriptor, const char* name);
        RHIAccelerationStructureRef CreateAccelerationStructure(const char* name);
        RHIShaderScope CreateShader(void* base, PK::Assets::Shader::PKShaderVariant* pVariant, const char* name);
        RHIWindowScope CreateWindowScope(const WindowDescriptor& descriptor);

        template<typename T>
        RHIBindArrayRef<T> CreateBindArray(size_t capacity);

        template<typename T>
        inline RHIBufferRef CreateBuffer(size_t count, BufferUsage usage, const char* name) { return CreateBuffer(sizeof(T) * count, usage, name); }

        bool ValidateTexture(RHITextureRef& inoutTexture, const TextureDescriptor& descriptor, const char* name);

        bool ValidateBuffer(RHIBufferRef& inoutBuffer, size_t size, BufferUsage usage, const char* name);

        template<typename T>
        inline bool ValidateBuffer(RHIBufferRef& inoutBuffer, size_t count, BufferUsage usage, const char* name) { return ValidateBuffer(inoutBuffer, sizeof(T) * count, usage, name); }

        void SetBuffer(NameID name, RHIBuffer* buffer, const BufferIndexRange& range);
        void SetBuffer(NameID name, RHIBuffer* buffer);
        void SetTexture(NameID name, RHITexture* texture, const TextureViewRange& range);
        void SetTexture(NameID name, RHITexture* texture, uint16_t level, uint16_t layer);
        void SetTexture(NameID name, RHITexture* texture);
        void SetImage(NameID name, RHITexture* texture, const TextureViewRange& range);
        void SetImage(NameID name, RHITexture* texture, uint16_t level, uint16_t layer);
        void SetImage(NameID name, RHITexture* texture);
        void SetSampler(NameID name, const SamplerDescriptor& sampler);
        void SetAccelerationStructure(NameID name, RHIAccelerationStructure* structure);
        void SetBufferArray(NameID name, RHIBindArray<RHIBuffer>* bufferArray);
        void SetTextureArray(NameID name, RHIBindArray<RHITexture>* textureArray);
        void SetConstant(NameID name, const void* data, uint32_t size);
        void SetKeyword(NameID name, bool value);

        template<typename T>
        void SetConstant(NameID name, const T& value) { SetConstant(name, &value, (uint32_t)sizeof(T)); }
    }
}
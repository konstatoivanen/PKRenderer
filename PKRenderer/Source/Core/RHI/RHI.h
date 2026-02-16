#pragma once
#include <cstdint>
#include "Core/Math/MathFwd.h"

namespace std
{
    template <class _Fty>
    class function;

    template <class, class>
    class unique_ptr;
}

namespace PKAssets
{
    enum class PKElementType : uint16_t;
    enum class PKDescriptorType : uint8_t;
    enum class PKShaderStage : uint8_t;  
    enum class PKShaderStageFlags : uint32_t; 
    enum class PKBlendFactor : uint8_t;  
    enum class PKBlendOp : uint8_t;  
    enum class PKColorMask  : uint8_t;  
    enum class PKComparison : uint8_t;  
    enum class PKCullMode : uint8_t;  
    enum class PKRasterMode : uint8_t; 
    enum class PKTextureType : uint8_t;
    enum class PKTextureFormat : uint8_t;
    enum class PKFilterMode : uint8_t;
    enum class PKWrapMode : uint8_t;  
    enum class PKBorderColor : uint8_t;
    struct PKShaderVariant;
}

namespace PK
{
    template<typename T>
    using Unique = std::unique_ptr<T>;

    template <typename T> struct Ref;
    template <typename T> struct Weak;

    struct NameID;
    struct FenceRef;
    class PropertyBlock;

    struct DrawIndexedIndirectCommand;
    struct DrawIndirectCommand;
    struct BuiltInResources;
    struct BufferIndexRange;
    struct TextureDataRegion;
    struct TextureViewRange;
    struct MultisamplingParameters;
    struct DepthStencilParameters;
    struct RasterizationParameters;
    struct BlendParameters;
    struct FixedFunctionShaderAttributes;
    struct FixedFunctionState;
    struct RenderTargetBinding;
    struct AccelerationStructureGeometryInfo;
    struct RHIDriverMemoryInfo;
    struct RHIDriverDescriptor;
    struct SwapchainDescriptor;
    struct SamplerDescriptor;
    struct TextureDescriptor;

    struct ShaderPushConstant;
    struct ShaderPushConstantLayout;
    struct ShaderResourceElement;
    struct ShaderResourceLayout;
    struct ShaderVertexInputLayout;
    struct VertexStreamElement;
    struct VertexStreamLayout;
    struct ShaderBindingTableInfo;
    
    typedef PKAssets::PKElementType ElementType;
    typedef PKAssets::PKDescriptorType ShaderResourceType;
    typedef PKAssets::PKShaderStage ShaderStage;
    typedef PKAssets::PKShaderStageFlags ShaderStageFlags;
    typedef PKAssets::PKBlendFactor BlendFactor;
    typedef PKAssets::PKBlendOp BlendOp;
    typedef PKAssets::PKColorMask ColorMask;
    typedef PKAssets::PKComparison Comparison;
    typedef PKAssets::PKCullMode CullMode;
    typedef PKAssets::PKRasterMode RasterMode;
    typedef PKAssets::PKTextureType TextureType;
    typedef PKAssets::PKTextureFormat TextureFormat;
    typedef PKAssets::PKFilterMode FilterMode;
    typedef PKAssets::PKWrapMode WrapMode;
    typedef PKAssets::PKBorderColor BorderColor;

    enum class RHIAPI;
    enum class QueueType;
    enum class SamplerType : uint8_t;
    enum class TextureBindMode : uint8_t;
    enum class PolygonMode : uint8_t;
    enum class Topology : uint8_t;
    enum class LogicOp : uint8_t;
    enum class FrontFace : uint8_t;
    enum class LoadOp : uint8_t;
    enum class StoreOp : uint8_t;
    enum class InputRate : uint8_t;
    enum class BufferUsage : uint32_t;
    enum class TextureUsage : uint16_t;
    enum class RayTracingShaderGroup;

    struct RHIDriver;
    struct RHISwapchain;
    struct RHIAccelerationStructure;
    template <typename>
    struct RHIBindArray;
    struct RHIBuffer;
    struct RHICommandBuffer;
    struct RHIQueueSet;
    struct RHIShader;
    struct RHITexture;
    template<typename T>
    using RHIBindArrayRef = Ref<RHIBindArray<T>>;

    typedef RHIBindArray<RHITexture> RHITextureBindArray;
    typedef RHIBindArray<RHIBuffer> RHIBufferBindArray;

    typedef Ref<RHIAccelerationStructure> RHIAccelerationStructureRef;
    typedef Ref<RHITextureBindArray> RHITextureBindArrayRef;
    typedef Ref<RHIBufferBindArray> RHIBufferBindArrayRef;
    typedef Ref<RHIBuffer> RHIBufferRef;
    typedef Ref<RHITexture> RHITextureRef;
    typedef Unique<RHISwapchain> RHISwapchainScope;
    typedef Unique<RHIShader> RHIShaderScope;
    typedef Unique<RHIDriver> RHIDriverScope;

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
        void WaitForIdle();
        void GC();

        RHIDriverScope CreateDriver(const char* workingDirectory, const RHIDriverDescriptor& descriptor);
        RHIBufferRef CreateBuffer(size_t size, BufferUsage usage, const char* name);
        RHITextureRef CreateTexture(const TextureDescriptor& descriptor, const char* name);
        RHIAccelerationStructureRef CreateAccelerationStructure(const char* name);
        RHIShaderScope CreateShader(void* base, PKAssets::PKShaderVariant* pVariant, const char* name);
        RHISwapchainScope CreateSwapchain(const SwapchainDescriptor& descriptor);

        RHIBuffer* AcquireStage(size_t size);
        void ReleaseStage(RHIBuffer* buffer, const FenceRef& fence);

        template<typename T>
        RHIBindArrayRef<T> CreateBindArray(size_t capacity);

        template<typename T>
        inline RHIBufferRef CreateBuffer(size_t count, BufferUsage usage, const char* name) { return CreateBuffer(sizeof(T) * count, usage, name); }

        bool ValidateTexture(RHITextureRef& inoutTexture, const TextureDescriptor& descriptor, const char* name);
        bool ValidateTexture(RHITextureRef& inoutTexture, const uint3& resolution);
        bool ValidateTexture(RHITextureRef& inoutTexture, const uint3& resolution, const uint32_t levels);
        bool ValidateTexture(RHITextureRef& inoutTexture, const uint32_t levels, const uint32_t layers);

        bool ValidateBuffer(RHIBufferRef& inoutBuffer, size_t size, BufferUsage usage, const char* name);
        bool ValidateBuffer(RHIBufferRef& inoutBuffer, size_t size);

        template<typename T>
        inline bool ValidateBuffer(RHIBufferRef& inoutBuffer, size_t count, BufferUsage usage, const char* name) { return ValidateBuffer(inoutBuffer, sizeof(T) * count, usage, name); }

        template<typename T> 
        inline bool ValidateBuffer(RHIBufferRef& inoutBuffer, size_t count) { return ValidateBuffer(inoutBuffer, sizeof(T) * count); }

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

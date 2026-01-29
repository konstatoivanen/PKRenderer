#pragma once
#include <PKAssets/PKAsset.h>
#include "Core/Utilities/NameID.h"
#include "Core/RHI/RHI.h"
#include "Core/Math/Math.h"

namespace PK
{
    constexpr static const uint32_t PK_RHI_MAX_MESHLETS_PER_TASK = 32;
    constexpr static const uint32_t PK_RHI_MAX_FRAMES_IN_FLIGHT = 2;
    constexpr static const uint32_t PK_RHI_MAX_SWAP_CHAIN_IMAGE_COUNT = 16;
    constexpr static const uint32_t PK_RHI_MAX_RENDER_TARGETS = 8;
    constexpr static const uint32_t PK_RHI_MAX_DESCRIPTOR_SETS = PKAssets::PK_ASSET_MAX_DESCRIPTOR_SETS;
    constexpr static const uint32_t PK_RHI_MAX_DESCRIPTORS_PER_SET = PKAssets::PK_ASSET_MAX_DESCRIPTORS_PER_SET;
    constexpr static const uint32_t PK_RHI_MAX_PUSH_CONSTANTS = PKAssets::PK_ASSET_MAX_PUSH_CONSTANTS;
    constexpr static const uint32_t PK_RHI_MAX_VERTEX_ATTRIBUTES = PKAssets::PK_ASSET_MAX_VERTEX_ATTRIBUTES;
    constexpr static const uint32_t PK_RHI_MAX_UNBOUNDED_SIZE = PKAssets::PK_ASSET_MAX_UNBOUNDED_SIZE;
    constexpr static const uint32_t PK_RHI_MAX_VIEWPORTS = 16;

    constexpr const static char* PK_RHI_SHADER_ENTRY_POINT_NAME = PKAssets::PK_SHADER_ENTRY_POINT_NAME;
    constexpr const static char* PK_RHI_VS_POSITION = PKAssets::PK_MESH_VS_POSITION;
    constexpr const static char* PK_RHI_VS_NORMAL = PKAssets::PK_MESH_VS_NORMAL;
    constexpr const static char* PK_RHI_VS_TANGENT = PKAssets::PK_MESH_VS_TANGENT;
    constexpr const static char* PK_RHI_VS_COLOR = PKAssets::PK_MESH_VS_COLOR;
    constexpr const static char* PK_RHI_VS_TEXCOORD0 = PKAssets::PK_MESH_VS_TEXCOORD0;
    constexpr const static char* PK_RHI_VS_TEXCOORD1 = PKAssets::PK_MESH_VS_TEXCOORD1;
    constexpr const static char* PK_RHI_VS_TEXCOORD2 = PKAssets::PK_MESH_VS_TEXCOORD2;
    constexpr const static char* PK_RHI_VS_TEXCOORD3 = PKAssets::PK_MESH_VS_TEXCOORD3;

    constexpr const static char PK_RHI_CUBE_FACE_RIGHT = 0;
    constexpr const static char PK_RHI_CUBE_FACE_LEFT = 1;
    constexpr const static char PK_RHI_CUBE_FACE_DOWN = 2;
    constexpr const static char PK_RHI_CUBE_FACE_UP = 3;
    constexpr const static char PK_RHI_CUBE_FACE_FRONT = 4;
    constexpr const static char PK_RHI_CUBE_FACE_BACK = 5;

    enum class RHIAPI
    {
        None,
        Vulkan,
        DX12
    };

    enum class QueueType
    {
        Transfer,
        Graphics,
        Compute,
        Present,
        MaxCount
    };

    enum class TextureBindMode : uint8_t
    {
        SampledTexture,
        Image,
        RenderTarget
    };

    enum class PolygonMode : uint8_t
    {
        Fill,
        Line,
        Point,
    };

    enum class Topology : uint8_t
    {
        PointList,
        LineList,
        LineStrip,
        TriangleList,
        TriangleStrip,
        TriangleFan,
        LineListWithAdjacency,
        LineStripWithAdjacency,
        TriangleListWithAdjacency,
        TriangleStripWithAdjacency,
        PatchList
    };

    enum class LogicOp : uint8_t
    {
        Clear,
        And,
        AndReverse,
        Copy,
        AndInverted,
        None,
        XOR,
        OR,
        NOR,
        Equal,
        Invert,
        OrReverse,
        CopyInverted,
        OrInverted,
        NAND,
        Set
    };

    enum class FrontFace : uint8_t
    {
        CounterClockwise,
        Clockwise,
    };

    enum class LoadOp : uint8_t
    {
        None,
        Load,
        Clear,
        Discard,
    };

    enum class StoreOp : uint8_t
    {
        None,
        Store,
        Discard,
    };

    enum class InputRate : uint8_t
    {
        PerVertex,
        PerInstance
    };

    enum class BufferUsage : uint32_t
    {
        None = 0,

        GPUOnly     = 1,
        CPUOnly     = 2,
        CPUToGPU    = 3,
        GPUToCPU    = 4,
        CPUCopy     = 5,

        TransferDst           = 1 << 4,
        TransferSrc           = 1 << 5,
        Vertex                = 1 << 6,
        Index                 = 1 << 7,
        Constant              = 1 << 8,
        Storage               = 1 << 9,
        Indirect              = 1 << 10,
        PersistentStage       = 1 << 11,
        Sparse                = 1 << 12,
        AccelerationStructure = 1 << 13,
        InstanceInput         = 1 << 14,
        ShaderBindingTable    = 1 << 15,
        Concurrent            = 1 << 16,

        TypeBits = 7,
        AlignedTypes = Storage | Constant,
        DefaultVertex = GPUOnly | TransferDst | Vertex,
        SparseVertex = DefaultVertex | Sparse,
        DefaultIndex = GPUOnly | TransferDst | Index,
        SparseIndex = DefaultIndex | Sparse,
        DefaultConstant = GPUOnly | TransferDst | Constant,
        DefaultStorage = GPUOnly | TransferDst | Storage,
        PersistentStorage = DefaultStorage | PersistentStage,
        DefaultStaging = CPUOnly | TransferSrc,
        DefaultShaderBindingTable = GPUOnly | TransferDst | ShaderBindingTable,
        DefaultAccelerationStructure = GPUOnly | AccelerationStructure
    };

    enum class TextureUsage : uint16_t
    {
        None = 0x0,
        RTColor = 0x1,
        RTDepth = 0x2,
        RTStencil = 0x4,
        Upload = 0x8,
        Sample = 0x10,
        Input = 0x20,
        Storage = 0x40,
        Concurrent = 0x80,
        ReadOnly = 0x100,
        Transient = 0x200,
        DefaultDisk = Upload | Sample | ReadOnly,
        Default = Upload | Sample,
        DefaultStorage = Upload | Sample | Storage,
        RTColorSample = RTColor | Sample,
        RTDepthSample = RTDepth | Sample,

        ValidRTTypes = RTColor | RTDepth | RTStencil,
    };

    enum class ColorSpace : uint8_t
    {
        sRGB_NonLinear,
        sRGB_Linear,
        scRGB,
        P3_NonLinear,
        P3_Linear,
        P3_DCI_NonLinear,
        BT709_Linear,
        BT709_NonLinear,
        BT2020_Linear,
        HDR10_ST2084,
        HDR10_HLG,
        DolbyVision,
        AdobeRGB_Linear,
        AdobeRGB_NonLinear,
        PassThrough,
        AmdFreeSync2
    };

    enum class VSyncMode : uint8_t
    {
        Immediate,
        Mailbox,
        Fifo,
        FifoRelaxed,
        FifoLatest,
        SharedDemandRefresh,
        SharedContinuous
    };

    enum class RayTracingShaderGroup
    {
        RayGeneration,
        Miss,
        Hit,
        Callable, //@TODO Add callable support
        MaxCount
    };

    constexpr const static ShaderStageFlags PK_RHI_RAYTRACING_GROUP_SHADER_STAGE[(uint32_t)RayTracingShaderGroup::MaxCount + 1] =
    {
        ShaderStageFlags::RayTraceGroupGeneration,
        ShaderStageFlags::RayTraceGroupMiss,
        ShaderStageFlags::RayTraceGroupHit,
        ShaderStageFlags::RayTraceGroupCallable,
        ShaderStageFlags::None
    };

    constexpr const static RayTracingShaderGroup PK_RHI_SHADER_STAGE_RAYTRACING_GROUP[(uint32_t)ShaderStage::MaxCount] =
    {
        RayTracingShaderGroup::MaxCount,        //Vertex,
        RayTracingShaderGroup::MaxCount,        //TesselationControl,
        RayTracingShaderGroup::MaxCount,        //TesselationEvaluation,
        RayTracingShaderGroup::MaxCount,        //Geometry,
        RayTracingShaderGroup::MaxCount,        //Fragment,
        RayTracingShaderGroup::MaxCount,        //Compute,
        RayTracingShaderGroup::MaxCount,        //MeshTask
        RayTracingShaderGroup::MaxCount,        //MeshAssembly,
        RayTracingShaderGroup::RayGeneration,   //RayGeneration,
        RayTracingShaderGroup::Miss,            //RayMiss,
        RayTracingShaderGroup::Hit,             //RayClosestHit,
        RayTracingShaderGroup::Hit,             //RayAnyHit,
        RayTracingShaderGroup::Hit,             //RayIntersection,
    };


    #define PK_DECLARE_ENUM_OPERATORS(Type) \
    static constexpr Type operator | (const Type& a, const Type& b) { return (Type)((uint32_t)a | (uint32_t)b); } \
    static constexpr Type operator |= (const Type& a, const Type& b) { return a | b; } \
    static constexpr Type operator & (const Type& a, const Type& b) { return (Type)((uint32_t)a & (uint32_t)b); } \
    static constexpr Type operator & (const Type& a, const uint32_t& b) { return (Type)((uint32_t)a & b); } \
    static constexpr bool operator == (const Type& a, const uint32_t& b) { return (uint32_t)a == b; } \
    static constexpr bool operator != (const Type& a, const uint32_t& b) { return (uint32_t)a != b; } \

    PK_DECLARE_ENUM_OPERATORS(ColorMask)
    PK_DECLARE_ENUM_OPERATORS(BufferUsage)
    PK_DECLARE_ENUM_OPERATORS(TextureUsage)
    PK_DECLARE_ENUM_OPERATORS(ShaderStageFlags)

    #undef PK_DECLARE_ENUM_OPERATORS

    struct TextureClearValue
    {
        private: 
            struct Depth { float depth; uint32_t stencil; };
        public:

        constexpr TextureClearValue() : float32(PK_FLOAT4_ZERO) {}
        constexpr TextureClearValue(const float4& v) : float32(v) {}
        constexpr TextureClearValue(const uint4& v) : uint32(v) {}
        constexpr TextureClearValue(const int4& v) : int32(v) {}
        constexpr TextureClearValue(const float depth, uint32_t stencil) : depth({ depth, stencil }) {}

        union
        {
            float4 float32;
            uint4 uint32;
            int4 int32;
            Depth depth;
        };

        constexpr bool operator == (const TextureClearValue& r) const noexcept
        {
            return uint32[0] == r.uint32[0] && 
                   uint32[1] == r.uint32[1] && 
                   uint32[2] == r.uint32[2] && 
                   uint32[3] == r.uint32[3];
        }

        constexpr bool operator != (const TextureClearValue& r) const noexcept
        {
            return !(*this == r);
        }
    };

    struct DrawIndexedIndirectCommand
    {
        uint32_t indexCount;
        uint32_t instanceCount;
        uint32_t firstIndex;
        int32_t vertexOffset;
        uint32_t firstInstance;
    };

    struct DrawIndirectCommand
    {
        uint32_t vertexCount;
        uint32_t instanceCount;
        uint32_t firstVertex;
        uint32_t firstInstance;
    };

    struct BufferIndexRange
    {
        size_t offset = 0;
        size_t count = 0;

        constexpr bool operator < (const BufferIndexRange& other) const
        {
            return offset != other.offset ? offset < other.offset : count < other.count;
        }

        constexpr bool operator == (const BufferIndexRange& other) const
        {
            return offset == other.offset && count == other.count;
        }
    };

    struct TextureDataRegion
    {
        size_t bufferOffset = 0ull;
        uint32_t level = 0ull;
        uint32_t layer = 0ull;
        uint32_t layers = 0ull;
        uint3 offset = PK_UINT3_ZERO;
        uint3 extent = PK_UINT3_ZERO;
    };

    struct TextureViewRange
    {
        uint16_t level = 0u;
        uint16_t layer = 0u;
        uint16_t levels = 0u;
        uint16_t layers = 0u;

        constexpr TextureViewRange() {}
        constexpr TextureViewRange(uint16_t level, uint16_t layer, uint16_t levels, uint16_t layers) : level(level), layer(layer), levels(levels), layers(layers) {}
    };

    struct MultisamplingParameters
    {
        bool sampleShadingEnable = false;
        bool alphaToCoverageEnable = false;
        bool alphaToOneEnable = false;
        uint8_t rasterizationSamples = 1u;
        float minSampleShading = 1.0f;
    };

    struct DepthStencilParameters
    {
        Comparison depthCompareOp = Comparison::Always;
        bool depthWriteEnable = false;
        bool depthBoundsTestEnable = false;
        bool stencilTestEnable = false;
        float minDepthBounds = 0.0f;
        float maxDepthBounds = 1.0f;
    };

    struct RasterizationParameters
    {
        bool depthClampEnable = false;
        bool rasterizerDiscardEnable = false;
        bool depthBiasEnable = false;
        PolygonMode polygonMode = PolygonMode::Fill;
        CullMode cullMode = CullMode::Off;
        FrontFace frontFace = FrontFace::CounterClockwise;
        RasterMode rasterMode = RasterMode::Default;
        Topology topology = Topology::TriangleList;
        float overEstimation = 0.0f;
        float depthBiasConstantFactor = 0;
        float depthBiasClamp = 0;
        float depthBiasSlopeFactor = 0;
        float lineWidth = 1.0f;
    };

    struct BlendParameters
    {
        ColorMask colorMask = ColorMask::RGBA;
        LogicOp logicOp = LogicOp::None;
        BlendFactor srcColorFactor = BlendFactor::None;
        BlendFactor dstColorFactor = BlendFactor::None;
        BlendOp colorOp = BlendOp::None;
        BlendFactor srcAlphaFactor = BlendFactor::None;
        BlendFactor dstAlphaFactor = BlendFactor::None;
        BlendOp alphaOp = BlendOp::None;

        inline bool isBlendEnabled() const { return colorOp != BlendOp::None || alphaOp != BlendOp::None ||
                                                    srcColorFactor != BlendFactor::None || 
                                                    dstColorFactor != BlendFactor::None || 
                                                    srcAlphaFactor != BlendFactor::None || 
                                                    dstAlphaFactor != BlendFactor::None; }

        inline bool isLogicOpEnabled() const { return logicOp != LogicOp::None; }
    };

    // Limited set of values that can be defined in the shader source
    struct FixedFunctionShaderAttributes
    {
        BlendParameters blending{};
        DepthStencilParameters depthStencil{};
        RasterizationParameters rasterization{};
    };

    struct RenderTargetBinding
    {
        struct RHITexture* target = nullptr;
        TextureViewRange targetRange{};
        struct RHITexture* resolve = nullptr;
        TextureViewRange resolveRange{};
        LoadOp loadOp = LoadOp::None;
        StoreOp storeOp = StoreOp::None;
        TextureClearValue clearValue{};

        constexpr RenderTargetBinding(RHITexture* target,
            const TextureViewRange& targetRange,
            RHITexture* resolve,
            const TextureViewRange& resolveRange,
            LoadOp loadOp,
            StoreOp storeOp,
            const TextureClearValue& clearValue) :
            target(target),
            targetRange(targetRange),
            resolve(resolve),
            resolveRange(resolveRange),
            loadOp(loadOp),
            storeOp(storeOp),
            clearValue(clearValue)
        {}

        constexpr RenderTargetBinding(RHITexture* target, RHITexture* resolve, LoadOp loadOp, StoreOp storeOp, const TextureClearValue& clearValue) : RenderTargetBinding(target, {}, resolve, {}, loadOp, storeOp, clearValue) {}
        constexpr RenderTargetBinding(RHITexture* target, RHITexture* resolve, LoadOp loadOp, StoreOp storeOp) : RenderTargetBinding(target, resolve, loadOp, storeOp, {}) {}
        constexpr RenderTargetBinding(RHITexture* target, const TextureViewRange& targetRange, LoadOp loadOp, StoreOp storeOp, const TextureClearValue& clearValue) : RenderTargetBinding(target, targetRange, nullptr, {}, loadOp, storeOp, clearValue) {}
        constexpr RenderTargetBinding(RHITexture* target, LoadOp loadOp, StoreOp storeOp, const TextureClearValue& clearValue) : RenderTargetBinding(target, nullptr, loadOp, storeOp, clearValue) {}
        constexpr RenderTargetBinding(RHITexture* target, LoadOp loadOp, StoreOp storeOp) : RenderTargetBinding(target, nullptr, loadOp, storeOp) {}
        constexpr RenderTargetBinding() {}
    };

    struct AccelerationStructureGeometryInfo
    {
        NameID name;
        RHIBuffer* vertexBuffer;
        RHIBuffer* indexBuffer;
        uint32_t vertexOffset;
        uint32_t vertexStride;
        uint32_t vertexFirst;
        uint32_t vertexCount;
        uint32_t indexStride;
        uint32_t indexFirst;
        uint32_t indexCount;
        uint32_t customIndex;
        uint32_t recordOffset;
    };

    struct RHIDriverMemoryInfo
    {
        uint32_t blockCount;
        uint32_t allocationCount;
        uint32_t unusedRangeCount;
        size_t usedBytes;
        size_t unusedBytes;
        size_t allocationSizeMin;
        size_t allocationSizeAvg;
        size_t allocationSizeMax;
        size_t unusedRangeSizeMin;
        size_t unusedRangeSizeAvg;
        size_t unusedRangeSizeMax;
    };

    struct RHIDriverDescriptor
    {
        RHIAPI api;
        uint32_t apiVersionMajor;
        uint32_t apiVersionMinor;
        uint32_t gcPruneDelay;
        bool enableValidation;
        bool enableDebugNames;
        bool enableDebugLabels;
        bool discardPipelineCache;
    };

    struct SwapchainDescriptor
    {
        uint2 desiredResolution = PK_UINT2_ZERO;
        uint desiredImageCount = 4u;
        TextureFormat desiredFormat = TextureFormat::BGRA8;
        ColorSpace desiredColorSpace = ColorSpace::sRGB_NonLinear;
        VSyncMode desiredVSyncMode = VSyncMode::Fifo;
        const void* nativeMonitorHandle = nullptr;
        void* nativeWindowHandle = nullptr;

        inline bool operator == (const SwapchainDescriptor& other) const noexcept
        {
            return memcmp(this, &other, sizeof(SwapchainDescriptor)) == 0;
        }

        inline bool operator != (const SwapchainDescriptor& other) const noexcept
        {
            return memcmp(this, &other, sizeof(SwapchainDescriptor)) != 0;
        }
    };

    struct SamplerDescriptor
    {
        FilterMode filterMin = FilterMode::Point;
        FilterMode filterMag = FilterMode::Point;
        WrapMode wrap[3] = { WrapMode::Clamp, WrapMode::Clamp, WrapMode::Clamp };
        Comparison comparison = Comparison::Off;
        BorderColor borderColor = BorderColor::FloatClear;
        bool normalized = true;
        float anisotropy = 0.0f;
        float mipBias = 0.0f;
        float mipMin = 0.0f;
        float mipMax = 0.0f;
    
        inline bool operator == (const SamplerDescriptor& other) const noexcept
        {
            return memcmp(this, &other, sizeof(SamplerDescriptor)) == 0;
        }

        inline bool operator != (const SamplerDescriptor& other) const noexcept
        {
            return memcmp(this, &other, sizeof(SamplerDescriptor)) != 0;
        }
    };

    struct TextureDescriptor
    {
        TextureFormat format = TextureFormat::RGBA8;
        TextureFormat formatAlias = TextureFormat::Invalid;
        TextureUsage usage = TextureUsage::Default;
        TextureType type = TextureType::Texture2D;
        uint3 resolution = PK_UINT3_ONE;
        uint8_t levels = 1;
        uint8_t samples = 1;
        uint16_t layers = 1;
        SamplerDescriptor sampler = {};
    };

    namespace RHIEnumConvert
    {
        constexpr auto Size = PKAssets::PKElementTypeToSize;
        constexpr auto Alignment = PKAssets::PKElementTypeToAlignment;
        constexpr auto IsResourceHandle = PKAssets::PKElementTypeIsResourceHandle;
        constexpr auto StringToElementType = PKAssets::StringToPKElementType;
        constexpr auto StringToShaderResourceType = PKAssets::StringToPKDescriptorType;
        constexpr auto StringToTextureType = PKAssets::StringToPKTextureType;
        constexpr auto StringToTextureFormat = PKAssets::StringToPKTextureFormat;
        constexpr auto StringToFilterMode = PKAssets::StringToPKFilterMode;
        constexpr auto StringToWrapMode = PKAssets::StringToPKWrapMode;
        constexpr auto StringToBorderColor = PKAssets::StringToPKBorderColor;
        constexpr auto StringToShaderStage = PKAssets::StringToPKShaderStage;
        constexpr auto StringToComparison = PKAssets::StringToPKComparison;
        constexpr auto StringToCullMode = PKAssets::StringToPKCullMode;
        constexpr auto StringToBlendFactor = PKAssets::StringToPKBlendFactor;
        constexpr auto StringToBlendOp = PKAssets::StringToPKBlendOp;
        constexpr auto StringToColorMask = PKAssets::StringToPKColorMask;
        constexpr auto StringToRasterMode = PKAssets::StringToPKRasterMode;

        RHIAPI StringToRHIAPI(const char* str);
        QueueType StringToQueueType(const char* str);
        TextureBindMode StringToTextureBindMode(const char* str);
        PolygonMode StringToPolygonMode(const char* str);
        Topology StringToTopology(const char* str);
        LogicOp StringToLogicOp(const char* str);
        FrontFace StringToFrontFace(const char* str);
        LoadOp StringToLoadOp(const char* str);
        StoreOp StringToStoreOp(const char* str);
        InputRate StringToInputRate(const char* str);
        TextureUsage StringToTextureUsage(const char* str);
        ColorSpace StringToColorSpace(const char* str);
        VSyncMode StringToVSyncMode(const char* str);
        RayTracingShaderGroup StringToRayTracingShaderGroup(const char* str);
    }
}

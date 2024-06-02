#pragma once
#include <PKAssets/PKAsset.h>
#include "Core/Utilities/NameID.h"
#include "Core/Math/Math.h"

namespace PK
{
    struct RHIBuffer;

    typedef PKAssets::PKElementType       ElementType;
    typedef PKAssets::PKDescriptorType    ShaderResourceType;
    typedef PKAssets::PKShaderStage       ShaderStage;
    typedef PKAssets::PKShaderStageFlags  ShaderStageFlags;
    typedef PKAssets::PKBlendFactor       BlendFactor;
    typedef PKAssets::PKBlendOp           BlendOp;
    typedef PKAssets::PKComparison        Comparison;
    typedef PKAssets::PKCullMode          CullMode;
    typedef PKAssets::PKRasterMode        RasterMode;

    constexpr static const uint32_t PK_RHI_DESIRED_SWAP_CHAIN_IMAGE_COUNT = 4;
    constexpr static const uint32_t PK_RHI_MAX_FRAMES_IN_FLIGHT = 2;
    constexpr static const uint32_t PK_RHI_MAX_RENDER_TARGETS = 8;
    constexpr static const uint32_t PK_RHI_MAX_DESCRIPTOR_SETS = PKAssets::PK_ASSET_MAX_DESCRIPTOR_SETS;
    constexpr static const uint32_t PK_RHI_MAX_DESCRIPTORS_PER_SET = PKAssets::PK_ASSET_MAX_DESCRIPTORS_PER_SET;
    constexpr static const uint32_t PK_RHI_MAX_PUSH_CONSTANTS = PKAssets::PK_ASSET_MAX_PUSH_CONSTANTS;
    constexpr static const uint32_t PK_RHI_MAX_VERTEX_ATTRIBUTES = PKAssets::PK_ASSET_MAX_VERTEX_ATTRIBUTES;
    constexpr static const uint32_t PK_RHI_MAX_UNBOUNDED_SIZE = PKAssets::PK_ASSET_MAX_UNBOUNDED_SIZE;
    constexpr static const uint32_t PK_RHI_MAX_VIEWPORTS = 16;

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
        Off,
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

    enum class SamplerType : uint8_t
    {
        Sampler2D,
        Sampler2DArray,
        Sampler3D,
        Cubemap,
        CubemapArray,
    };

    enum class TextureBindMode : uint8_t
    {
        SampledTexture,
        Image,
        RenderTarget
    };

    enum class FilterMode : uint8_t
    {
        Point,
        Bilinear,
        Trilinear,
        Bicubic
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

    enum class WrapMode : uint8_t
    {
        Clamp,
        Repeat,
        Mirror,
        MirrorOnce,
        Border
    };

    enum class ColorMask : uint8_t
    {
        NONE = 0u,
        R = 0x1u,
        G = 0x2u,
        B = 0x4u,
        A = 0x8u,
        RG = R | G,
        RGB = RG | B,
        RGBA = RGB | A,
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
        Keep,
        Clear,
        Discard,
    };

    enum class StoreOp : uint8_t
    {
        Store,
        Discard,
    };

    enum class BorderColor : uint8_t
    {
        FloatClear,
        IntClear,
        FloatBlack,
        IntBlack,
        FloatWhite,
        IntWhite
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
        DefaultDisk = Upload | Sample | ReadOnly,
        Default = Upload | Sample,
        DefaultStorage = Upload | Sample | Storage,
        RTColorSample = RTColor | Sample,
        RTDepthSample = RTDepth | Sample,

        ValidRTTypes = RTColor | RTDepth | RTStencil,
        ValidRTColorUsages = Sample | Storage | Input | Concurrent,
        ValidRTDepthUsages = Sample | Input | Concurrent,
    };

    enum class TextureFormat : uint16_t
    {
        Invalid = 0,
        R8, 
        R8_SNORM, 
        R8UI, 
        R8I, 
        Stencil8,
        R16F, 
        R16UI, 
        R16I,
        RG8, 
        RG8_SNORM, 
        RG8UI, 
        RG8I,
        RGB565,
        RGB9E5, // 9995 is actually 32 bpp but it's here for historical reasons.
        RGB5A1,
        RGBA4,
        Depth16,
        RGB8, 
        RGB8_SRGB, 
        RGB8_SNORM, 
        RGB8UI, 
        RGB8I,
        R32F, 
        R32UI, 
        R32I,
        RG16F, 
        RG16UI, 
        RG16I,
        B10G11R11UF,
        RGBA8, 
        RGBA8_SRGB, 
        RGBA8_SNORM, 
        BGRA8_SRGB,
        RGB10A2, 
        RGBA8UI, 
        RGBA8I,
        Depth32F, 
        Depth24_Stencil8, 
        Depth32F_Stencil8,
        RGB16F, 
        RGB16UI, 
        RGB16I,
        RG32F, 
        RG32UI, 
        RG32I,
        RGBA16, 
        RGBA16F, 
        RGBA16UI, 
        RGBA16I,
        RGB32F, 
        RGB32UI, 
        RGB32I,
        RGBA32F, 
        RGBA32UI, 
        RGBA32I,
        RGBA64UI,

        // compressed formats
        BC1_RGB, 
        BC1_RGBA, 
        BC1_SRGB, 
        BC1_SRGBA, 
        BC4, 
        BC2_RGBA, 
        BC2_SRGBA, 
        BC3_RGBA,
        BC3_SRGBA,
        BC6H_RGBUF,
        BC6H_RGBF,
        BC7_UNORM,
    };

    //@TODO Should not need these enums
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

    namespace ElementTypeConvert
    {
        inline uint16_t Size(ElementType format) { return (uint16_t)PKAssets::GetElementSize(format); }
        inline uint16_t Alignment(ElementType format) { return (uint16_t)PKAssets::GetElementAlignment(format); }
        inline uint16_t Components(ElementType format) { return (uint16_t)PKAssets::GetElementComponents(format); }
    }

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

    struct TextureUploadRange
    {
        size_t bufferOffset;
        uint32_t level;
        uint32_t layer;
        uint32_t layers;
        uint3 offset;
        uint3 extent;
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

    struct FixedFunctionState
    {
        RasterizationParameters rasterization{};
        BlendParameters blending{};
        DepthStencilParameters depthStencil{};
        MultisamplingParameters multisampling{};
        uint8_t colorTargetCount = 0u;
        uint8_t viewportCount = 1u;
        uint16_t excludeStageMask = 0u;
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
    };

    // Descriptors
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
    };

    struct TextureDescriptor
    {
        TextureFormat format = TextureFormat::RGBA8;
        TextureFormat formatAlias = TextureFormat::Invalid;
        TextureUsage usage = TextureUsage::Default;
        SamplerType samplerType = SamplerType::Sampler2D;
        uint3 resolution = PK_UINT3_ONE;
        uint8_t levels = 1;
        uint8_t samples = 1;
        uint16_t layers = 1;
        SamplerDescriptor sampler = {};
    };

    struct WindowDescriptor
    {
        std::string title;
        std::string iconPath;
        uint32_t width;
        uint32_t height;
        bool vsync;
        bool cursorVisible;

        WindowDescriptor(const std::string& title = "PK Window",
            const std::string& iconPath = std::string(),
            uint32_t width = 1600u,
            uint32_t height = 912u,
            bool vsync = true,
            bool cursorVisible = true) :
            title(title),
            iconPath(iconPath),
            width(width),
            height(height),
            vsync(vsync),
            cursorVisible(cursorVisible)
        {
        }
    };
}

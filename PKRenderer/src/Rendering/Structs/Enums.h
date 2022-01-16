#pragma once
#include "PrecompiledHeader.h"
#include <PKAssets/PKAsset.h>

namespace PK::Rendering::Structs
{
    typedef PK::Assets::PKDescriptorType ResourceType;
    typedef PK::Assets::PKElementType ElementType;
    typedef PK::Assets::PKShaderStage ShaderStage;
    typedef PK::Assets::PKBlendFactor BlendFactor;
    typedef PK::Assets::PKBlendOp BlendOp;
    typedef PK::Assets::PKComparison Comparison;
    typedef PK::Assets::PKCullMode CullMode;
    typedef PK::Assets::Shader::Type ShaderType;

    constexpr static const uint32_t PK_DESIRED_SWAP_CHAIN_IMAGE_COUNT = 4;
    constexpr static const uint32_t PK_MAX_FRAMES_IN_FLIGHT = 2;
    constexpr static const uint32_t PK_MAX_RENDER_TARGETS = 8;
    constexpr static const uint32_t PK_SHADOW_CASCADE_COUNT = 4;
    constexpr static const uint32_t PK_MAX_DESCRIPTOR_SETS = PK::Assets::PK_ASSET_MAX_DESCRIPTOR_SETS;
    constexpr static const uint32_t PK_MAX_DESCRIPTORS_PER_SET = PK::Assets::PK_ASSET_MAX_DESCRIPTORS_PER_SET;
    constexpr static const uint32_t PK_MAX_VERTEX_ATTRIBUTES = PK::Assets::PK_ASSET_MAX_VERTEX_ATTRIBUTES;
    constexpr static const uint32_t PK_MAX_UNBOUNDED_SIZE = PK::Assets::PK_ASSET_MAX_UNBOUNDED_SIZE;

    constexpr const static char* PK_VS_POSITION = PK::Assets::Mesh::PK_VS_POSITION;
    constexpr const static char* PK_VS_NORMAL = PK::Assets::Mesh::PK_VS_NORMAL;
    constexpr const static char* PK_VS_TANGENT = PK::Assets::Mesh::PK_VS_TANGENT;  
    constexpr const static char* PK_VS_COLOR = PK::Assets::Mesh::PK_VS_COLOR; 
    constexpr const static char* PK_VS_TEXCOORD0 = PK::Assets::Mesh::PK_VS_TEXCOORD0;
    constexpr const static char* PK_VS_TEXCOORD1 = PK::Assets::Mesh::PK_VS_TEXCOORD1;
    constexpr const static char* PK_VS_TEXCOORD2 = PK::Assets::Mesh::PK_VS_TEXCOORD2;
    constexpr const static char* PK_VS_TEXCOORD3 = PK::Assets::Mesh::PK_VS_TEXCOORD3;

    constexpr const static char PK_CUBE_FACE_RIGHT = 0;
    constexpr const static char PK_CUBE_FACE_LEFT = 1;
    constexpr const static char PK_CUBE_FACE_DOWN = 2;
    constexpr const static char PK_CUBE_FACE_UP = 3;
    constexpr const static char PK_CUBE_FACE_FRONT = 4;
    constexpr const static char PK_CUBE_FACE_BACK = 5;

    enum class APIType
    {
        Off,
        Vulkan,
        DX12
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

    enum class WrapMode : uint8_t
    {
        Clamp,
        Repeat,
        Mirror,
        MirrorOnce,
        Border
    };

    enum class ColorMask
    {
        NONE = 0,
        R = 0x00000001,
        G = 0x00000002,
        B = 0x00000004,
        A = 0x00000008,
        RG = R | G,
        RGB = RG | B,
        RGBA = RGB | A,
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

    enum class MemoryAccessFlags
    {
        None                 = 0,
        StageIndirect        = 1 << 0,
        StageVertexInput     = 1 << 1,
        StageVertex          = 1 << 2,
        StageGeometry        = 1 << 3,
        StageFragment        = 1 << 4,
        StageCompute         = 1 << 5,
        StageDepthStencil    = 1 << 6, // Early depth & stencil test
        StageDepthStencilOut = 1 << 7, // depth & stencil write
        StageColorOut        = 1 << 8, // Color write
        
        ReadShader   = 1 << 9, // Read textures, images, buffers
        ReadUniform  = 1 << 10, // Read uniform buffers
        ReadVertex   = 1 << 11, // Read vertex buffer
        ReadIndex    = 1 << 12, // Read index buffer
        ReadIndirect = 1 << 13, // Read indirect arguments
        ReadRTColor  = 1 << 14,
        ReadRTDepth  = 1 << 15,

        WriteShader  = 1 << 16, // Write textures, images, buffers
        WriteRTColor = 1 << 17,
        WriteRTDepth = 1 << 18,

        ReadWriteShader = ReadShader | WriteShader, // Read/Write, images & buffers
        ReadWriteRTColor = ReadRTColor | WriteRTColor,
        ReadWriteRTDepth = ReadRTDepth | WriteRTDepth,

        // short hands
        FragmentAttachmentColor = ReadWriteRTColor | StageColorOut,         // Write color in fragment out
        FragmentAttachmentDepth = ReadWriteRTDepth | StageDepthStencilOut,  // Write depth in fragment out
        FragmentTexture = ReadShader | StageFragment,                       // Read texture in fragment
        FragmentBuffer = ReadShader | StageFragment,                       // Read biffer in fragment
        ComputeReadWrite = ReadWriteShader | StageCompute,                  // Read/Write texture, image, & buffer  in compute
        ComputeRead = ReadShader | StageCompute,                            // Read texture, image, & buffer in compute
        ComputeWrite = WriteShader | StageCompute,                           // Write texture, image, & buffer  in compute
    };

    enum class BufferUsage : uint8_t
    {
        None = 0x0,
        Vertex = 0x1,
        Index = 0x2,
        Staging = 0x4,
        Constant = 0x8,
        Storage = 0x10,
        Readback = 0x20,
        
        Dynamic = 0x20,
        PersistentStage = 0x40,
        ExtraFlags = Dynamic | PersistentStage,
    };

    enum class TextureUsage : uint8_t
    {
        None = 0x0,
        RTColor = 0x1,
        RTDepth = 0x2,
        RTStencil = 0x4,
        Upload = 0x8,
        Sample = 0x10,
        Input = 0x20,
        Storage = 0x40,
        Default = Upload | Sample,
        DefaultStorage = Upload | Sample | Storage,
        RTColorSample = RTColor | Sample,
        RTDepthSample = RTDepth | Sample,

        ValidRTTypes = RTColor | RTDepth | RTStencil,
        ValidRTColorUsages = Sample | Storage | Input,
        ValidRTDepthUsages = Sample | Input,
    };

    enum class RenderableFlags : uint8_t
    {
        Mesh = 1 << 0,
        Light = 1 << 1,
        Static = 1 << 2,
        CastShadows = 1 << 3,
        Cullable = 1 << 4,

        // Presets
        DefaultMesh = Mesh | Static | CastShadows | Cullable,
        DefaultMeshNoShadows = Mesh | Static | Cullable,
    };

    enum class LightType : uint8_t
    {
        Point = 0,
        Spot = 1,
        Directional = 2,
        TypeCount
    };

    enum class Cookie : uint8_t
    {
        Circle0,
        Circle1,
        Circle2,
        Square0,
        Square1,
        Square2,
        Triangle,
        Star,
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
        R11FG11FB10F,
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

        // compressed formats

        // Mandatory in GLES 3.0 and GL 4.3
        EAC_R11, 
        EAC_R11_SIGNED, 
        EAC_RG11, 
        EAC_RG11_SIGNED,
        ETC2_RGB8, 
        ETC2_SRGB8,
        ETC2_RGB8_A1, 
        ETC2_SRGB8_A1,
        ETC2_RGBA8, 
        ETC2_SRGBA8, 

        // Available everywhere except Android/iOS
        DXT4, 
        DXT1_RGB, 
        DXT1_RGBA, 
        DXT3_RGBA, 
        DXT5_RGBA,
        DXT1_SRGB, 
        DXT1_SRGBA, 
        DXT3_SRGBA, 
        DXT5_SRGBA,
        
        // ASTC formats are available with a GLES extension
        RGBA_ASTC_4x4,
        RGBA_ASTC_5x4,
        RGBA_ASTC_5x5,
        RGBA_ASTC_6x5,
        RGBA_ASTC_6x6,
        RGBA_ASTC_8x5,
        RGBA_ASTC_8x6,
        RGBA_ASTC_8x8,
        RGBA_ASTC_10x5,
        RGBA_ASTC_10x6,
        RGBA_ASTC_10x8,
        RGBA_ASTC_10x10,
        RGBA_ASTC_12x10,
        RGBA_ASTC_12x12,
        SRGB8_ALPHA8_ASTC_4x4,
        SRGB8_ALPHA8_ASTC_5x4,
        SRGB8_ALPHA8_ASTC_5x5,
        SRGB8_ALPHA8_ASTC_6x5,
        SRGB8_ALPHA8_ASTC_6x6,
        SRGB8_ALPHA8_ASTC_8x5,
        SRGB8_ALPHA8_ASTC_8x6,
        SRGB8_ALPHA8_ASTC_8x8,
        SRGB8_ALPHA8_ASTC_10x5,
        SRGB8_ALPHA8_ASTC_10x6,
        SRGB8_ALPHA8_ASTC_10x8,
        SRGB8_ALPHA8_ASTC_10x10,
        SRGB8_ALPHA8_ASTC_12x10,
        SRGB8_ALPHA8_ASTC_12x12,
    };

    #define PK_DECLARE_ENUM_OPERATORS(Type) \
    static constexpr Type operator | (const Type& a, const Type& b) { return (Type)((uint32_t)a | (uint32_t)b); } \
    static constexpr Type operator |= (const Type& a, const Type& b) { return a | b; } \
    static constexpr Type operator & (const Type& a, const Type& b) { return (Type)((uint32_t)a & (uint32_t)b); } \
    static constexpr Type operator & (const Type& a, const int& b) { return (Type)((uint32_t)a & (uint32_t)b); } \
    static constexpr bool operator == (const Type& a, const int& b) { return (uint32_t)a == b; } \
    static constexpr bool operator != (const Type& a, const int& b) { return (uint32_t)a != b; } \

    PK_DECLARE_ENUM_OPERATORS(ColorMask)
    PK_DECLARE_ENUM_OPERATORS(MemoryAccessFlags)
    PK_DECLARE_ENUM_OPERATORS(BufferUsage)
    PK_DECLARE_ENUM_OPERATORS(TextureUsage)
    PK_DECLARE_ENUM_OPERATORS(RenderableFlags)

    #undef PK_DECLARE_ENUM_OPERATORS

    namespace ElementConvert
    {
        inline uint16_t Size(ElementType format) { return (uint16_t)PK::Assets::GetElementSize(format); }
        inline uint16_t Alignment(ElementType format) { return (uint16_t)PK::Assets::GetElementAlignment(format); }
        inline uint16_t Components(ElementType format) { return (uint16_t)PK::Assets::GetElementComponents(format); }
    }
}
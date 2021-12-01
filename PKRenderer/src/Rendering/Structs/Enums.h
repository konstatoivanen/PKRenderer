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

    constexpr const static uint32_t PK_DESIRED_SWAP_CHAIN_IMAGE_COUNT = 4;
    constexpr static const int PK_MAX_FRAMES_IN_FLIGHT = 2;
    constexpr static const int PK_MAX_RENDER_TARGETS = 8;
    constexpr static const uint32_t PK_MAX_DESCRIPTOR_SETS = 4;
    constexpr static const uint32_t PK_MAX_DESCRIPTORS_PER_SET = 16;
    constexpr const static uint32_t PK_MAX_VERTEX_ATTRIBUTES = 8;
    
    constexpr const static char* PK_VS_POSIITON = "in_POSITION";
    constexpr const static char* PK_VS_NORMAL = "in_NORMAL";
    constexpr const static char* PK_VS_TANGENT = "in_TANGENT";
    constexpr const static char* PK_VS_COLOR = "in_COLOR";
    constexpr const static char* PK_VS_TEXCOORD0 = "in_TEXCOORD0";
    constexpr const static char* PK_VS_TEXCOORD1 = "in_TEXCOORD1";
    constexpr const static char* PK_VS_TEXCOORD2 = "in_TEXCOORD2";
    constexpr const static char* PK_VS_TEXCOORD3 = "in_TEXCOORD3";

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

    typedef enum ColorMask
    {
        PK_COLOR_MASK_NONE = 0,
        PK_COLOR_MASK_R = 0x00000001,
        PK_COLOR_MASK_G = 0x00000002,
        PK_COLOR_MASK_B = 0x00000004,
        PK_COLOR_MASK_A = 0x00000008,
        PK_COLOR_MASK_RG = PK_COLOR_MASK_R | PK_COLOR_MASK_G,
        PK_COLOR_MASK_RGB = PK_COLOR_MASK_RG | PK_COLOR_MASK_B,
        PK_COLOR_MASK_RGBA = PK_COLOR_MASK_RGB | PK_COLOR_MASK_A,
    } ColorMask;

    enum class FrontFace : uint8_t
    {
        CounterClockwise,
        Clockwise,
    };

    enum class LoadOp : uint16_t
    {
        Keep,
        Clear,
        Discard,
    };

    enum class StoreOp : uint16_t
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

    enum class BufferUsage : uint8_t
    {
        None = 0x0,
        Vertex = 0x1,
        Index = 0x2,
        Staging = 0x4,
        Uniform = 0x8,
        Storage = 0x10,
        Dynamic = 0x20,
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
        Default = Upload | Sample
    };

    enum class TextureFormat : uint16_t 
    {
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
        ETC2_EAC_RGBA8, 
        ETC2_EAC_SRGBA8,

        // Available everywhere except Android/iOS
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

    namespace ElementConvert
    {
        uint16_t Size(ElementType format);
        uint16_t Alignment(ElementType format);
        uint16_t Components(ElementType format);
    }
}
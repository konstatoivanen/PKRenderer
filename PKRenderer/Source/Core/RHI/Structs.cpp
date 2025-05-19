#include "PrecompiledHeader.h"
#include "Structs.h"

namespace PK
{

    const static char* RHIAPI_NAMES[] =
    {
        "None",
        "Vulkan",
        "DX12"
    };

    const static char* QueueType_NAMES[] = 
    {
        "Transfer",
        "Graphics",
        "Compute",
        "Present",
        "MaxCount"
    };

    const static char* TextureType_NAMES[] =
    {
        "Texture2D",
        "Texture2DArray",
        "Texture3D",
        "Cubemap",
        "CubemapArray",
    };

    const static char* TextureBindMode_NAMES[] =
    {
        "SampledTexture",
        "Image",
        "RenderTarget"
    };

    const static char* Comparison_NAMES[] =
    {
        "Off",
        "Never",
        "Less",
        "Equal",
        "LessEqual",
        "Greater",
        "NotEqual",
        "GreaterEqual",
        "Always",
    };

    const static char* FilterMode_NAMES[] =
    {
        "Point",
        "Bilinear",
        "Trilinear",
        "Bicubic"
    };

    const static char* PolygonMode_NAMES[] =
    {
        "Fill",
        "Line",
        "Point",
    };

    const static char* Topology_NAMES[] =
    {
        "PointList",
        "LineList",
        "LineStrip",
        "TriangleList",
        "TriangleStrip",
        "TriangleFan",
        "LineListWithAdjacency",
        "LineStripWithAdjacency",
        "TriangleListWithAdjacency",
        "TriangleStripWithAdjacency",
        "PatchList"
    };

    const static char* WrapMode_NAMES[] =
    {
        "Clamp",
        "Repeat",
        "Mirror",
        "MirrorOnce",
        "Border"
    };

    const static char* LogicOp_NAMES[] =
    {
        "Clear",
        "And",
        "AndReverse",
        "Copy",
        "AndInverted",
        "None",
        "XOR",
        "OR",
        "NOR",
        "Equal",
        "Invert",
        "OrReverse",
        "CopyInverted",
        "OrInverted",
        "NAND",
        "Set"
    };

    const static char* FrontFace_NAMES[] =
    {
        "CounterClockwise",
        "Clockwise",
    };

    const static char* LoadOp_NAMES[] =
    {
        "Keep",
        "Clear",
        "Discard",
    };

    const static char* StoreOp_NAMES[] =
    {
        "Store",
        "Discard",
    };

    const static char* BorderColor_NAMES[] =
    {
        "FloatClear",
        "IntClear",
        "FloatBlack",
        "IntBlack",
        "FloatWhite",
        "IntWhite"
    };

    const static char* InputRate_NAMES[] =
    {
        "PerVertex",
        "PerInstance"
    };

    const static char* TextureFormat_NAMES[] =
    {
        "Invalid",
        "R8",
        "R8_SNORM",
        "R8UI",
        "R8I",
        "Stencil8",
        "R16F",
        "R16UI",
        "R16I",
        "RG8",
        "RG8_SNORM",
        "RG8UI",
        "RG8I",
        "RGB565",
        "RGB9E5",
        "RGB5A1",
        "RGBA4",
        "Depth16",
        "RGB8",
        "RGB8_SRGB",
        "RGB8_SNORM",
        "RGB8UI",
        "RGB8I",
        "R32F",
        "R32UI",
        "R32I",
        "RG16F",
        "RG16UI",
        "RG16I",
        "B10G11R11UF",
        "RGBA8",
        "RGBA8_SRGB",
        "RGBA8_SNORM",
        "BGRA8",
        "BGRA8_SRGB",
        "RGB10A2",
        "RGBA8UI",
        "RGBA8I",
        "Depth32F",
        "Depth24_Stencil8",
        "Depth32F_Stencil8",
        "RGB16F",
        "RGB16UI",
        "RGB16I",
        "RG32F",
        "RG32UI",
        "RG32I",
        "RGBA16",
        "RGBA16F",
        "RGBA16UI",
        "RGBA16I",
        "RGB32F",
        "RGB32UI",
        "RGB32I",
        "RGBA32F",
        "RGBA32UI",
        "RGBA32I",
        "RGBA64UI",
        "BC1_RGB",
        "BC1_RGBA",
        "BC1_SRGB",
        "BC1_SRGBA",
        "BC4",
        "BC2_RGBA",
        "BC2_SRGBA",
        "BC3_RGBA",
        "BC3_SRGBA",
        "BC6H_RGBUF",
        "BC6H_RGBF",
        "BC7_UNORM",
    };

    const static char* ColorSpace_NAMES[] =
    {
        "sRGB_NonLinear",
        "sRGB_Linear",
        "scRGB",
        "P3_NonLinear",
        "P3_Linear",
        "P3_DCI_NonLinear",
        "BT709_Linear",
        "BT709_NonLinear",
        "BT2020_Linear",
        "HDR10_ST2084",
        "HDR10_HLG",
        "DolbyVision",
        "AdobeRGB_Linear",
        "AdobeRGB_NonLinear",
        "PassThrough",
        "AmdFreeSync2"
    };

    const static char* VSyncMode_NAMES[] =
    {
        "Immediate",
        "Mailbox",
        "Fifo",
        "FifoRelaxed",
        "FifoLatest",
        "SharedDemandRefresh",
        "SharedContinuous"
    };

    const static char* RayTracingShaderGroup_NAMES[] =
    {
        "RayGeneration",
        "Miss",
        "Hit",
        "Callable",
        "MaxCount"
    };

    template<size_t size>
    uint32_t FindEnumIndexFromString(const char* (&arr)[size], const char* str, uint32_t fallback)
    {
        for (auto i = 0u; str && i < size; ++i)
        {
            if (strncmp(arr[i], str, 20) == 0)
            {
                return i;
            }
        }

        return fallback;
    }

    #define DECLARE_RHI_STRING_TO_ENUM(TType, TFallback) TType RHIEnumConvert::StringTo##TType(const char* str) { return (TType)FindEnumIndexFromString(TType##_NAMES, str, (uint32_t)TFallback); }

    Comparison StringToComparison(const char* str);

    DECLARE_RHI_STRING_TO_ENUM(RHIAPI, RHIAPI::None)
    DECLARE_RHI_STRING_TO_ENUM(QueueType, QueueType::MaxCount)
    DECLARE_RHI_STRING_TO_ENUM(TextureType, TextureType::Texture2D)
    DECLARE_RHI_STRING_TO_ENUM(TextureBindMode, TextureBindMode::SampledTexture)
    DECLARE_RHI_STRING_TO_ENUM(Comparison, Comparison::Off)
    DECLARE_RHI_STRING_TO_ENUM(FilterMode, FilterMode::Point)
    DECLARE_RHI_STRING_TO_ENUM(PolygonMode, PolygonMode::Fill)
    DECLARE_RHI_STRING_TO_ENUM(Topology, Topology::PointList)
    DECLARE_RHI_STRING_TO_ENUM(WrapMode, WrapMode::Clamp)
    DECLARE_RHI_STRING_TO_ENUM(LogicOp, LogicOp::Clear)
    DECLARE_RHI_STRING_TO_ENUM(FrontFace, FrontFace::CounterClockwise)
    DECLARE_RHI_STRING_TO_ENUM(LoadOp, LoadOp::Keep)
    DECLARE_RHI_STRING_TO_ENUM(StoreOp, StoreOp::Store)
    DECLARE_RHI_STRING_TO_ENUM(BorderColor, BorderColor::FloatClear)
    DECLARE_RHI_STRING_TO_ENUM(InputRate, InputRate::PerVertex)
    DECLARE_RHI_STRING_TO_ENUM(TextureFormat, TextureFormat::Invalid)
    DECLARE_RHI_STRING_TO_ENUM(ColorSpace, ColorSpace::sRGB_NonLinear)
    DECLARE_RHI_STRING_TO_ENUM(VSyncMode, VSyncMode::Immediate)
    DECLARE_RHI_STRING_TO_ENUM(RayTracingShaderGroup, RayTracingShaderGroup::RayGeneration)

    ColorMask RHIEnumConvert::StringToColorMask(const char* str)
    {
        auto length = strnlen(str, 4);
        
        ColorMask mask = ColorMask::NONE;

        for (auto i = 0u; i < length; ++i)
        {
            switch (str[i])
            {
                case 'X':
                case 'R': mask = mask | ColorMask::R; break;
                case 'Y':
                case 'G': mask = mask | ColorMask::G; break;
                case 'Z':
                case 'B': mask = mask | ColorMask::B; break;
                case 'W':
                case 'A': mask = mask | ColorMask::B; break;
                default: break;
            }
        }

        return mask;
    }

    TextureUsage RHIEnumConvert::StringToTextureUsage(const char* str)
    {
        TextureUsage usage = TextureUsage::None;

        if (strstr(str, "RTColor"))
        {
            usage = usage | TextureUsage::RTColor;
        }

        if (strstr(str, "RTDepth"))
        {
            usage = usage | TextureUsage::RTDepth;
        }

        if (strstr(str, "RTStencil"))
        {
            usage = usage | TextureUsage::RTStencil;
        }

        if (strstr(str, "Upload"))
        {
            usage = usage | TextureUsage::Upload;
        }

        if (strstr(str, "Sample"))
        {
            usage = usage | TextureUsage::Sample;
        }

        if (strstr(str, "Input"))
        {
            usage = usage | TextureUsage::Input;
        }

        if (strstr(str, "Storage"))
        {
            usage = usage | TextureUsage::Storage;
        }

        if (strstr(str, "Concurrent"))
        {
            usage = usage | TextureUsage::Concurrent;
        }

        if (strstr(str, "ReadOnly"))
        {
            usage = usage | TextureUsage::ReadOnly;
        }

        if (strstr(str, "DefaultDisk"))
        {
            usage = usage | TextureUsage::DefaultDisk;
        }

        if (strstr(str, "Default"))
        {
            usage = usage | TextureUsage::Default;
        }

        return usage;
    }

    #undef DECLARE_RHI_STRING_TO_ENUM
}

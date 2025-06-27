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

    const static char* TextureBindMode_NAMES[] =
    {
        "SampledTexture",
        "Image",
        "RenderTarget"
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
        "None",
        "Load",
        "Clear",
        "Discard",
    };

    const static char* StoreOp_NAMES[] =
    {
        "None",
        "Store",
        "Discard",
    };

    const static char* InputRate_NAMES[] =
    {
        "PerVertex",
        "PerInstance"
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
        auto length = str ? strnlen(str, 64) : 0u;

        for (auto i = 0u; length && i < size; ++i)
        {
            if (strncmp(arr[i], str, length) == 0)
            {
                return i;
            }
        }

        return fallback;
    }

    #define DECLARE_RHI_STRING_TO_ENUM(TType, TFallback) TType RHIEnumConvert::StringTo##TType(const char* str) { return (TType)FindEnumIndexFromString(TType##_NAMES, str, (uint32_t)TFallback); }

    DECLARE_RHI_STRING_TO_ENUM(RHIAPI, RHIAPI::None)
    DECLARE_RHI_STRING_TO_ENUM(QueueType, QueueType::MaxCount)
    DECLARE_RHI_STRING_TO_ENUM(TextureBindMode, TextureBindMode::SampledTexture)
    DECLARE_RHI_STRING_TO_ENUM(PolygonMode, PolygonMode::Fill)
    DECLARE_RHI_STRING_TO_ENUM(Topology, Topology::PointList)
    DECLARE_RHI_STRING_TO_ENUM(LogicOp, LogicOp::Clear)
    DECLARE_RHI_STRING_TO_ENUM(FrontFace, FrontFace::CounterClockwise)
    DECLARE_RHI_STRING_TO_ENUM(LoadOp, LoadOp::None)
    DECLARE_RHI_STRING_TO_ENUM(StoreOp, StoreOp::Store)
    DECLARE_RHI_STRING_TO_ENUM(InputRate, InputRate::PerVertex)
    DECLARE_RHI_STRING_TO_ENUM(ColorSpace, ColorSpace::sRGB_NonLinear)
    DECLARE_RHI_STRING_TO_ENUM(VSyncMode, VSyncMode::Immediate)
    DECLARE_RHI_STRING_TO_ENUM(RayTracingShaderGroup, RayTracingShaderGroup::RayGeneration)

    TextureUsage RHIEnumConvert::StringToTextureUsage(const char* str)
    {
        TextureUsage usage = TextureUsage::None;

        if (!str || strnlen(str, 128) == 0)
        {
            return usage;
        }

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

        if (strstr(str, "Transient"))
        {
            usage = usage | TextureUsage::Transient;
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

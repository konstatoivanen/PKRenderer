#include "PrecompiledHeader.h"
#include "Core/RHI/Structs.h"
#include "Core/Yaml/RapidyamlPrivate.h"

namespace PK::YAML
{
    #define DECLARE_RHI_ENUM_READ(TType)                    \
    template<>                                              \
    bool Read<TType>(const ConstNode& node, TType* rhs)     \
    {                                                       \
        auto value = node.val();                            \
        FixedString128 valuestr(value.len, value.data());   \
        *rhs = RHIEnumConvert::StringTo##TType(valuestr);   \
        return true;                                        \
    }                                                       \
    PK_YAML_DECLARE_READ_MEMBER(TType)                      \

    DECLARE_RHI_ENUM_READ(ElementType)
    DECLARE_RHI_ENUM_READ(RHIAPI)
    DECLARE_RHI_ENUM_READ(QueueType)
    DECLARE_RHI_ENUM_READ(TextureType)
    DECLARE_RHI_ENUM_READ(TextureBindMode)
    DECLARE_RHI_ENUM_READ(Comparison)
    DECLARE_RHI_ENUM_READ(FilterMode)
    DECLARE_RHI_ENUM_READ(PolygonMode)
    DECLARE_RHI_ENUM_READ(Topology)
    DECLARE_RHI_ENUM_READ(WrapMode)
    DECLARE_RHI_ENUM_READ(ColorMask)
    DECLARE_RHI_ENUM_READ(LogicOp)
    DECLARE_RHI_ENUM_READ(FrontFace)
    DECLARE_RHI_ENUM_READ(LoadOp)
    DECLARE_RHI_ENUM_READ(StoreOp)
    DECLARE_RHI_ENUM_READ(BorderColor)
    DECLARE_RHI_ENUM_READ(InputRate)
    DECLARE_RHI_ENUM_READ(TextureUsage)
    DECLARE_RHI_ENUM_READ(TextureFormat)
    DECLARE_RHI_ENUM_READ(ColorSpace)
    DECLARE_RHI_ENUM_READ(VSyncMode)
    DECLARE_RHI_ENUM_READ(RayTracingShaderGroup)

    template<>
    bool Read<RHIDriverDescriptor>(const ConstNode& node, RHIDriverDescriptor* rhs)
    {
        bool isValid = true;
        isValid &= YAML::Read<RHIAPI>(node, "RHIDriverDescriptor.api", &rhs->api);
        isValid &= YAML::Read<uint32_t>(node, "RHIDriverDescriptor.apiVersionMajor", &rhs->apiVersionMajor);
        isValid &= YAML::Read<uint32_t>(node, "RHIDriverDescriptor.apiVersionMinor", &rhs->apiVersionMinor);
        isValid &= YAML::Read<uint32_t>(node, "RHIDriverDescriptor.gcPruneDelay", &rhs->gcPruneDelay);
        isValid &= YAML::Read<bool>(node, "RHIDriverDescriptor.enableValidation", &rhs->enableValidation);
        isValid &= YAML::Read<bool>(node, "RHIDriverDescriptor.enableDebugNames", &rhs->enableDebugNames);
        isValid &= YAML::Read<bool>(node, "RHIDriverDescriptor.discardPipelineCache", &rhs->discardPipelineCache);
        return isValid;
    }

    PK_YAML_DECLARE_READ_MEMBER(RHIDriverDescriptor)

    template<>
    bool Read<SwapchainDescriptor>(const ConstNode& node, SwapchainDescriptor* rhs)
    {
        rhs->nativeMonitorHandle = nullptr;
        rhs->nativeWindowHandle = nullptr;
        bool isValid = true;
        isValid &= YAML::Read<uint2>(node, "SwapchainDescriptor.desiredResolution", &rhs->desiredResolution);
        isValid &= YAML::Read<uint32_t>(node, "SwapchainDescriptor.desiredImageCount", &rhs->desiredImageCount);
        isValid &= YAML::Read<TextureFormat>(node, "SwapchainDescriptor.desiredFormat", &rhs->desiredFormat);
        isValid &= YAML::Read<ColorSpace>(node, "SwapchainDescriptor.desiredColorSpace", &rhs->desiredColorSpace);
        isValid &= YAML::Read<VSyncMode>(node, "SwapchainDescriptor.desiredVSyncMode", &rhs->desiredVSyncMode);
        return isValid;
    }

    PK_YAML_DECLARE_READ_MEMBER(SwapchainDescriptor)

    template<>
    bool Read<SamplerDescriptor>(const ConstNode& node, SamplerDescriptor* rhs)
    {
        bool isValid = true;
        isValid &= YAML::Read<FilterMode>(node, "SamplerDescriptor.filterMin", &rhs->filterMin);
        isValid &= YAML::Read<FilterMode>(node, "SamplerDescriptor.filterMag", &rhs->filterMag);
        isValid &= YAML::Read<Comparison>(node, "SamplerDescriptor.comparison", &rhs->comparison);
        isValid &= YAML::Read<BorderColor>(node, "SamplerDescriptor.borderColor", &rhs->borderColor);
        isValid &= YAML::Read<bool>(node, "SamplerDescriptor.normalized", &rhs->normalized);
        isValid &= YAML::Read<float>(node, "SamplerDescriptor.anisotropy", &rhs->anisotropy);
        isValid &= YAML::Read<float>(node, "SamplerDescriptor.mipBias", &rhs->mipBias);
        isValid &= YAML::Read<float>(node, "SamplerDescriptor.mipMin", &rhs->mipMin);
        isValid &= YAML::Read<float>(node, "SamplerDescriptor.mipMax", &rhs->mipMax);

        auto nodeWrap = node.find_child("SamplerDescriptor.wrap");

        if (nodeWrap.readable() && nodeWrap.is_flow_sl())
        {
            for (auto i = 0u; i < 3u; ++i)
            {
                auto index = i < nodeWrap.num_children() ? i : nodeWrap.num_children() - 1u;
                isValid &= YAML::Read<WrapMode>(nodeWrap[index], &rhs->wrap[i]);
            }
        }

        return isValid;
    }

    PK_YAML_DECLARE_READ_MEMBER(SamplerDescriptor)

    template<>
    bool Read<TextureDescriptor>(const ConstNode& node, TextureDescriptor* rhs)
    {
        bool isValid = true;
        isValid &= YAML::Read<TextureFormat>(node, "TextureDescriptor.format", &rhs->format);
        isValid &= YAML::Read<TextureFormat>(node, "TextureDescriptor.formatAlias", &rhs->formatAlias);
        isValid &= YAML::Read<TextureUsage>(node, "TextureDescriptor.usage", &rhs->usage);
        isValid &= YAML::Read<TextureType>(node, "TextureDescriptor.type", &rhs->type);
        isValid &= YAML::Read<uint3>(node, "TextureDescriptor.resolution", &rhs->resolution);
        isValid &= YAML::Read<uint8_t>(node, "TextureDescriptor.levels", &rhs->levels);
        isValid &= YAML::Read<uint8_t>(node, "TextureDescriptor.samples", &rhs->samples);
        isValid &= YAML::Read<uint16_t>(node, "TextureDescriptor.layers", &rhs->layers);
        isValid &= YAML::Read<SamplerDescriptor>(node, "TextureDescriptor.sampler", &rhs->sampler);
        return isValid;
    }

    PK_YAML_DECLARE_READ_MEMBER(TextureDescriptor)
}

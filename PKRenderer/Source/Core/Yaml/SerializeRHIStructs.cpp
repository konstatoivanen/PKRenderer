#include "PrecompiledHeader.h"
#include "Core/Utilities/FixedString.h"
#include "Core/RHI/Structs.h"
#include "Core/Yaml/Serialize.h"

namespace PK::YAML
{
    #define DECLARE_RHI_ENUM_READ(TType)                                    \
    template<>                                                              \
    void Read<TType>(const ConstNode& node, TType* rhs)                     \
    {                                                                       \
        auto value = node.val();                                            \
        FixedString128 valuestr(value.len, value.data());                   \
        *rhs = RHIEnumConvert::StringTo##TType(valuestr);                   \
    }                                                                       \
    template<>                                                              \
    void Write<TType>(Node& node, const TType* rhs)                         \
    {                                                                       \
        node << RHIEnumConvert::TType##ToString(*rhs) |= ryml::VAL_DQUO;    \
    }                                                                       \

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
    void Read<RHIDriverDescriptor>(const ConstNode& node, RHIDriverDescriptor* rhs)
    {
        YAML::Read<RHIAPI>(node, "RHIDriverDescriptor.api", &rhs->api);
        YAML::Read<uint32_t>(node, "RHIDriverDescriptor.apiVersionMajor", &rhs->apiVersionMajor);
        YAML::Read<uint32_t>(node, "RHIDriverDescriptor.apiVersionMinor", &rhs->apiVersionMinor);
        YAML::Read<uint32_t>(node, "RHIDriverDescriptor.gcPruneDelay", &rhs->gcPruneDelay);
        YAML::Read<bool>(node, "RHIDriverDescriptor.enableValidation", &rhs->enableValidation);
        YAML::Read<bool>(node, "RHIDriverDescriptor.enableDebugNames", &rhs->enableDebugNames);
        YAML::Read<bool>(node, "RHIDriverDescriptor.enableDebugLabels", &rhs->enableDebugLabels);
        YAML::Read<bool>(node, "RHIDriverDescriptor.enableDebugShaderPrint", &rhs->enableDebugShaderPrint);
        YAML::Read<bool>(node, "RHIDriverDescriptor.enableDebugLogging", &rhs->enableDebugLogging);
        YAML::Read<bool>(node, "RHIDriverDescriptor.enablePipelineCache", &rhs->enablePipelineCache);
    }

    template<>
    void Write<RHIDriverDescriptor>(Node& node, const RHIDriverDescriptor* rhs)
    {
        node |= ryml::MAP;
        YAML::Write<RHIAPI>(node, "RHIDriverDescriptor.api", &rhs->api);
        YAML::Write<uint32_t>(node, "RHIDriverDescriptor.apiVersionMajor", &rhs->apiVersionMajor);
        YAML::Write<uint32_t>(node, "RHIDriverDescriptor.apiVersionMinor", &rhs->apiVersionMinor);
        YAML::Write<uint32_t>(node, "RHIDriverDescriptor.gcPruneDelay", &rhs->gcPruneDelay);
        YAML::Write<bool>(node, "RHIDriverDescriptor.enableValidation", &rhs->enableValidation);
        YAML::Write<bool>(node, "RHIDriverDescriptor.enableDebugNames", &rhs->enableDebugNames);
        YAML::Write<bool>(node, "RHIDriverDescriptor.enableDebugLabels", &rhs->enableDebugLabels);
        YAML::Write<bool>(node, "RHIDriverDescriptor.enableDebugShaderPrint", &rhs->enableDebugShaderPrint);
        YAML::Write<bool>(node, "RHIDriverDescriptor.enableDebugLogging", &rhs->enableDebugLogging);
        YAML::Write<bool>(node, "RHIDriverDescriptor.enablePipelineCache", &rhs->enablePipelineCache);
    }

    template<>
    void Read<SwapchainDescriptor>(const ConstNode& node, SwapchainDescriptor* rhs)
    {
        rhs->nativeMonitorHandle = nullptr;
        rhs->nativeWindowHandle = nullptr;
        YAML::Read<uint2>(node, "SwapchainDescriptor.desiredResolution", &rhs->desiredResolution);
        YAML::Read<uint32_t>(node, "SwapchainDescriptor.desiredImageCount", &rhs->desiredImageCount);
        YAML::Read<TextureFormat>(node, "SwapchainDescriptor.desiredFormat", &rhs->desiredFormat);
        YAML::Read<ColorSpace>(node, "SwapchainDescriptor.desiredColorSpace", &rhs->desiredColorSpace);
        YAML::Read<VSyncMode>(node, "SwapchainDescriptor.desiredVSyncMode", &rhs->desiredVSyncMode);
    }

    template<>
    void Write<SwapchainDescriptor>(Node& node, const SwapchainDescriptor* rhs)
    {
        node |= ryml::MAP;
        YAML::Write<uint2>(node, "SwapchainDescriptor.desiredResolution", &rhs->desiredResolution);
        YAML::Write<uint32_t>(node, "SwapchainDescriptor.desiredImageCount", &rhs->desiredImageCount);
        YAML::Write<TextureFormat>(node, "SwapchainDescriptor.desiredFormat", &rhs->desiredFormat);
        YAML::Write<ColorSpace>(node, "SwapchainDescriptor.desiredColorSpace", &rhs->desiredColorSpace);
        YAML::Write<VSyncMode>(node, "SwapchainDescriptor.desiredVSyncMode", &rhs->desiredVSyncMode);
    }

    template<>
    void Read<SamplerDescriptor>(const ConstNode& node, SamplerDescriptor* rhs)
    {
        YAML::Read<FilterMode>(node, "SamplerDescriptor.filterMin", &rhs->filterMin);
        YAML::Read<FilterMode>(node, "SamplerDescriptor.filterMag", &rhs->filterMag);
        YAML::Read<Comparison>(node, "SamplerDescriptor.comparison", &rhs->comparison);
        YAML::Read<BorderColor>(node, "SamplerDescriptor.borderColor", &rhs->borderColor);
        YAML::Read<bool>(node, "SamplerDescriptor.normalized", &rhs->normalized);
        YAML::Read<float>(node, "SamplerDescriptor.anisotropy", &rhs->anisotropy);
        YAML::Read<float>(node, "SamplerDescriptor.mipBias", &rhs->mipBias);
        YAML::Read<float>(node, "SamplerDescriptor.mipMin", &rhs->mipMin);
        YAML::Read<float>(node, "SamplerDescriptor.mipMax", &rhs->mipMax);

        auto nodeWrap = node.find_child("SamplerDescriptor.wrap");

        if (nodeWrap.readable() && nodeWrap.is_flow_sl())
        {
            for (auto i = 0u; i < 3u; ++i)
            {
                auto index = i < nodeWrap.num_children() ? i : nodeWrap.num_children() - 1u;
                YAML::Read<WrapMode>(nodeWrap[index], &rhs->wrap[i]);
            }
        }
    }

    template<>
    void Write<SamplerDescriptor>(Node& node, const SamplerDescriptor* rhs)
    {
        node |= ryml::MAP;
        YAML::Write<FilterMode>(node, "SamplerDescriptor.filterMin", &rhs->filterMin);
        YAML::Write<FilterMode>(node, "SamplerDescriptor.filterMag", &rhs->filterMag);
        YAML::Write<Comparison>(node, "SamplerDescriptor.comparison", &rhs->comparison);
        YAML::Write<BorderColor>(node, "SamplerDescriptor.borderColor", &rhs->borderColor);
        YAML::Write<bool>(node, "SamplerDescriptor.normalized", &rhs->normalized);
        YAML::Write<float>(node, "SamplerDescriptor.anisotropy", &rhs->anisotropy);
        YAML::Write<float>(node, "SamplerDescriptor.mipBias", &rhs->mipBias);
        YAML::Write<float>(node, "SamplerDescriptor.mipMin", &rhs->mipMin);
        YAML::Write<float>(node, "SamplerDescriptor.mipMax", &rhs->mipMax);

        auto nodeWrap = node["SamplerDescriptor.wrap"];
        nodeWrap |= ryml::SEQ | ryml::FLOW_SL;

        for (auto i = 0u; i < 3u; ++i)
        {
            nodeWrap.append_child() << RHIEnumConvert::WrapModeToString(rhs->wrap[i]) |= ryml::VAL_DQUO;
        }
    }

    template<>
    void Read<TextureDescriptor>(const ConstNode& node, TextureDescriptor* rhs)
    {
        YAML::Read<TextureFormat>(node, "TextureDescriptor.format", &rhs->format);
        YAML::Read<TextureFormat>(node, "TextureDescriptor.formatAlias", &rhs->formatAlias);
        YAML::Read<TextureUsage>(node, "TextureDescriptor.usage", &rhs->usage);
        YAML::Read<TextureType>(node, "TextureDescriptor.type", &rhs->type);
        YAML::Read<uint3>(node, "TextureDescriptor.resolution", &rhs->resolution);
        YAML::Read<uint8_t>(node, "TextureDescriptor.levels", &rhs->levels);
        YAML::Read<uint8_t>(node, "TextureDescriptor.samples", &rhs->samples);
        YAML::Read<uint16_t>(node, "TextureDescriptor.layers", &rhs->layers);
        YAML::Read<SamplerDescriptor>(node, "TextureDescriptor.sampler", &rhs->sampler);
    }

    template<>
    void Write<TextureDescriptor>(Node& node, const TextureDescriptor* rhs)
    {
        node |= ryml::MAP;
        YAML::Write<TextureFormat>(node, "TextureDescriptor.format", &rhs->format);
        YAML::Write<TextureFormat>(node, "TextureDescriptor.formatAlias", &rhs->formatAlias);
        YAML::Write<TextureUsage>(node, "TextureDescriptor.usage", &rhs->usage);
        YAML::Write<TextureType>(node, "TextureDescriptor.type", &rhs->type);
        YAML::Write<uint3>(node, "TextureDescriptor.resolution", &rhs->resolution);
        YAML::Write<uint8_t>(node, "TextureDescriptor.levels", &rhs->levels);
        YAML::Write<uint8_t>(node, "TextureDescriptor.samples", &rhs->samples);
        YAML::Write<uint16_t>(node, "TextureDescriptor.layers", &rhs->layers);
        YAML::Write<SamplerDescriptor>(node, "TextureDescriptor.sampler", &rhs->sampler);
    }
}

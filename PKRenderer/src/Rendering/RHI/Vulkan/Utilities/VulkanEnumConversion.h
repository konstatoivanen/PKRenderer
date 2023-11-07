#pragma once
#include "vulkan/vulkan.h"
#include "Rendering/RHI/Structs.h"

namespace PK::Rendering::RHI::Vulkan::EnumConvert
{
    VkFormat GetFormat(ElementType format);
    ElementType GetElementType(VkFormat format);
    VkFormat GetFormat(TextureFormat format);
    VkIndexType GetIndexType(ElementType format);
    TextureFormat GetTextureFormat(VkFormat format);
    bool IsDepthFormat(VkFormat format);
    bool IsDepthStencilFormat(VkFormat format);

    VkComponentMapping GetSwizzle(VkFormat format);
    VkImageViewType GetViewType(SamplerType samplerType);
    VkImageLayout GetImageLayout(TextureUsage usage);
    VkAttachmentLoadOp GetLoadOp(LoadOp loadOp);
    VkAttachmentLoadOp GetLoadOp(VkImageLayout layout, LoadOp loadOp);
    VkAttachmentStoreOp GetStoreOp(StoreOp storeOp);
    VkCompareOp GetCompareOp(Comparison comparison);
    VkBorderColor GetBorderColor(BorderColor color);
    VkSamplerAddressMode GetSamplerAddressMode(WrapMode wrap);
    VkFilter GetFilterMode(FilterMode filter);
    VkDescriptorType GetDescriptorType(ResourceType type);
    ResourceType GetResourceType(VkDescriptorType type, uint32_t count);
    VkShaderStageFlagBits GetShaderStage(ShaderStage stage);
    VkPipelineBindPoint GetPipelineBindPoint(ShaderStageFlags stageFlags);
    VkSampleCountFlagBits GetSampleCountFlags(uint32_t samples);
    VkVertexInputRate GetInputRate(InputRate inputRate);
    VkShaderStageFlagBits GetShaderStageFlags(ShaderStageFlags stageFlags);
    VkPolygonMode GetPolygonMode(PolygonMode mode);
    VkBlendOp GetBlendOp(BlendOp op);
    VkBlendFactor GetBlendFactor(BlendFactor factor, VkBlendFactor fallback);
    VkLogicOp GetLogicOp(LogicOp op);
    VkCullModeFlagBits GetCullMode(CullMode op);
    VkConservativeRasterizationModeEXT GetRasterMode(RasterMode mode);
    VkPrimitiveTopology GetTopology(Topology topology);
    VkFrontFace GetFrontFace(FrontFace face);
    VkPipelineStageFlags GetPipelineStageFlags(VkShaderStageFlags flags);
    VkRayTracingShaderGroupTypeKHR GetRayTracingStageGroupType(ShaderStage stage);
    VkImageAspectFlagBits GetFormatAspect(VkFormat format);
    uint32_t ExpandVkRange16(uint32_t v);
    bool IsReadAccess(VkAccessFlags flags);
    bool IsWriteAccess(VkAccessFlags flags);
}
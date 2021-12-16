#pragma once
#include "vulkan/vulkan.h"
#include "Rendering/Structs/Enums.h"

namespace PK::Rendering::VulkanRHI::EnumConvert
{
    using namespace PK::Rendering::Structs;

    VkFormat GetFormat(ElementType format);
    ElementType GetElementType(VkFormat format);
    VkFormat GetFormat(TextureFormat format);
    VkIndexType GetIndexType(ElementType format);
    TextureFormat GetTextureFormat(VkFormat format);
    bool IsDepthFormat(VkFormat format);

    VkComponentMapping GetSwizzle(VkFormat format);
    VkImageViewType GetViewType(SamplerType samplerType);
    VkImageLayout GetImageLayout(TextureUsage usage, bool useOptimized = false);
    VkAttachmentLoadOp GetLoadOp(LoadOp loadOp);
    VkAttachmentStoreOp GetStoreOp(StoreOp storeOp);
    VkCompareOp GetCompareOp(Comparison comparison);
    VkBorderColor GetBorderColor(BorderColor color);
    VkSamplerAddressMode GetSamplerAddressMode(WrapMode wrap);
    VkFilter GetFilterMode(FilterMode filter);
    VkDescriptorType GetDescriptorType(ResourceType type);
    ResourceType GetResourceType(VkDescriptorType type, uint32_t count);
    VkShaderStageFlagBits GetShaderStage(ShaderStage stage);
    VkPipelineBindPoint GetPipelineBindPoint(ShaderType type);
    VkSampleCountFlagBits GetSampleCountFlags(uint32_t samples);
    VkVertexInputRate GetInputRate(InputRate inputRate);
    VkShaderStageFlagBits GetShaderStageFlags(uint32_t pkStageFlags);
    VkPolygonMode GetPolygonMode(PolygonMode mode);
    VkBlendOp GetBlendOp(BlendOp op);
    VkBlendFactor GetBlendFactor(BlendFactor factor, VkBlendFactor fallback);
    VkCullModeFlagBits GetCullMode(CullMode op);
    VkFrontFace GetFrontFace(FrontFace face);
    VkPipelineStageFlagBits GetPipelineStageFlags(MemoryAccessFlags flags);
    VkAccessFlagBits GetAccessFlags(MemoryAccessFlags flags);
}
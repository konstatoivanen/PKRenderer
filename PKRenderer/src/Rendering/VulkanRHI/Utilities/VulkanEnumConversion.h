#pragma once
#include "vulkan/vulkan.h"
#include "Rendering/Structs/Enums.h"

namespace PK::Rendering::VulkanRHI::EnumConvert
{
    VkFormat GetFormat(Rendering::Structs::ElementType format);
    Rendering::Structs::ElementType GetElementType(VkFormat format);
    VkFormat GetFormat(Rendering::Structs::TextureFormat format);
    VkIndexType GetIndexType(Rendering::Structs::ElementType format);
    Rendering::Structs::TextureFormat GetTextureFormat(VkFormat format);
    bool IsDepthFormat(VkFormat format);
    bool IsDepthStencilFormat(VkFormat format);

    VkComponentMapping GetSwizzle(VkFormat format);
    VkImageViewType GetViewType(Rendering::Structs::SamplerType samplerType);
    VkImageLayout GetImageLayout(Rendering::Structs::TextureUsage usage, bool useOptimized = false);
    VkAttachmentLoadOp GetLoadOp(Rendering::Structs::LoadOp loadOp);
    VkAttachmentLoadOp GetLoadOp(VkImageLayout layout, Rendering::Structs::LoadOp loadOp);
    VkAttachmentStoreOp GetStoreOp(Rendering::Structs::StoreOp storeOp);
    VkCompareOp GetCompareOp(Rendering::Structs::Comparison comparison);
    VkBorderColor GetBorderColor(Rendering::Structs::BorderColor color);
    VkSamplerAddressMode GetSamplerAddressMode(Rendering::Structs::WrapMode wrap);
    VkFilter GetFilterMode(Rendering::Structs::FilterMode filter);
    VkDescriptorType GetDescriptorType(Rendering::Structs::ResourceType type);
    Rendering::Structs::ResourceType GetResourceType(VkDescriptorType type, uint32_t count);
    VkShaderStageFlagBits GetShaderStage(Rendering::Structs::ShaderStage stage);
    VkPipelineBindPoint GetPipelineBindPoint(Rendering::Structs::ShaderType type);
    VkSampleCountFlagBits GetSampleCountFlags(uint32_t samples);
    VkVertexInputRate GetInputRate(Rendering::Structs::InputRate inputRate);
    VkShaderStageFlagBits GetShaderStageFlags(uint32_t pkStageFlags);
    VkPolygonMode GetPolygonMode(Rendering::Structs::PolygonMode mode);
    VkBlendOp GetBlendOp(Rendering::Structs::BlendOp op);
    VkBlendFactor GetBlendFactor(Rendering::Structs::BlendFactor factor, VkBlendFactor fallback);
    VkCullModeFlagBits GetCullMode(Rendering::Structs::CullMode op);
    VkFrontFace GetFrontFace(Rendering::Structs::FrontFace face);
    VkPipelineStageFlags GetPipelineStageFlags(VkShaderStageFlags flags);
    VkRayTracingShaderGroupTypeKHR GetRayTracingStageGroupType(Rendering::Structs::ShaderStage stage);
    bool IsReadAccess(VkAccessFlags flags);
    bool IsWriteAccess(VkAccessFlags flags);
}
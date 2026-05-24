#pragma once
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanDriver;

    struct VulkanTexture : public RHITexture
    {
        VulkanTexture(struct VulkanDriver* driver, const TextureDescriptor& descriptor, const char* name);
        ~VulkanTexture();
        
        void SetSampler(const SamplerDescriptor& sampler) final;
        const TextureDescriptor& GetDescriptor() const final { return m_descriptor; }
        const char* GetDebugName() const final { return m_name.c_str(); }
        void* GetNativeHandle() const final { return m_image; }

        inline VkImageLayout GetImageLayout() const { return VulkanEnumConvert::GetImageLayout(m_descriptor.usage); }
        inline VkImageAspectFlags GetAspectFlags() const { return VulkanEnumConvert::GetFormatAspect(m_format); }
        inline const VulkanBindHandle* GetBindHandle() { return &GetView({})->bindHandle; }
        inline const VulkanBindHandle* GetBindHandle(TextureBindMode bindMode) { return &GetView({}, bindMode)->bindHandle; }
        inline const VulkanBindHandle* GetBindHandle(const TextureViewRange& range, TextureBindMode bindMode) { return &GetView(range, bindMode)->bindHandle; }
        void FillBindHandle(VulkanBindHandle* handle, const TextureViewRange& range, TextureBindMode bindMode) const;
        inline void FillBindHandle(VulkanBindHandle* handle, TextureBindMode bindMode) const { FillBindHandle(handle, {}, bindMode); }
        
    private:
        TextureViewRange NormalizeViewRange(const TextureViewRange& range) const;
        const VulkanImageView* GetView(const TextureViewRange& range, TextureBindMode mode = TextureBindMode::SampledTexture);

        const FixedString64 m_name;
        const VulkanDriver* m_driver;
        const VkFormat m_format;
        const VkFormat m_formatAlias;
        TextureDescriptor m_descriptor;
        VmaAllocation m_memory;
        VkImage m_image;
        VkImage m_imageAlias;
        LinkedList<VulkanImageView, uint64_t> m_firstView = nullptr;
    };
}

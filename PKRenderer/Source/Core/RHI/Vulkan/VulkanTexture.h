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
        const void* GetNativeView(const TextureViewRange& range, TextureViewMode viewMode);
        
    private:
        TextureViewRange NormalizeViewRange(const TextureViewRange& range) const;

        const FixedString64 m_name;
        const VulkanDriver* m_driver;
        const VkFormat m_format;
        const VkFormat m_formatAlias;
        TextureDescriptor m_descriptor;
        VkDeviceAddress m_deviceAddress;
        VmaAllocation m_memory;
        VkImage m_image;
        VkImage m_imageAlias;
        VulkanBindHandle m_rawHandle;
        LinkedList<VulkanImageView, uint64_t> m_firstView = nullptr;
    };
}

#pragma once
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanDriver;
    
    enum VulkanTextureBindMode
    {
        VulkanTextureBind_SRV,
        VulkanTextureBind_UAV,
        VulkanTextureBind_RTV
    };

    struct VulkanTexture : public RHITexture
    {
        VulkanTexture(struct VulkanDriver* driver, const TextureDescriptor& descriptor, const char* name);
        ~VulkanTexture();
        
        void SetSampler(const SamplerDescriptor& sampler) final;
        const TextureDescriptor& GetDescriptor() const final { return m_descriptor; }
        const char* GetDebugName() const final { return m_name.c_str(); }
        void* GetNativeHandle() const final { return m_image; }

        inline VkImageAspectFlags GetAspectFlags() const { return VulkanEnumConvert::GetFormatAspect(m_format); }
        inline const VulkanBindHandle* GetBindHandle() { return &GetView({}, VulkanTextureBind_SRV)->bindHandle; }
        inline const VulkanBindHandle* GetBindHandle(VulkanTextureBindMode bindMode) { return &GetView({}, bindMode)->bindHandle; }
        inline const VulkanBindHandle* GetBindHandle(const TextureViewRange& range, VulkanTextureBindMode bindMode) { return &GetView(range, bindMode)->bindHandle; }
        void FillBindHandle(VulkanBindHandle* handle, const TextureViewRange& range, VulkanTextureBindMode bindMode) const;
        inline void FillBindHandle(VulkanBindHandle* handle, VulkanTextureBindMode bindMode) const { FillBindHandle(handle, {}, bindMode); }
        
    private:
        TextureViewRange NormalizeViewRange(const TextureViewRange& range) const;
        const VulkanImageView* GetView(const TextureViewRange& range, VulkanTextureBindMode mode);

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

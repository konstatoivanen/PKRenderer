#pragma once
#include "Core/Base/Types/VersionedObject.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanDriver;

    struct VulkanTexture : public RHITexture, public VersionedObject
    {
        VulkanTexture(struct VulkanDriver* driver, const TextureDescriptor& descriptor, const char* name);
        ~VulkanTexture();

        void SetSampler(const SamplerDescriptor& sampler) final;
        const TextureDescriptor& GetDescriptor() const final { return m_descriptor; }
        const char* GetDebugName() const final { return m_name.c_str(); }
        const void* GetNativeView() const final { return &m_defaultView; }
        VulkanResourceView GetResourceView(const TextureViewRange& range, TextureViewMode viewMode) const;

    private:
        TextureViewRange NormalizeViewRange(const TextureViewRange& range) const;

        const FixedString64 m_name;
        const VulkanDriver* m_driver;
        const VkFormat m_format;
        const VkFormat m_formatAlias;
        TextureDescriptor m_descriptor;
        uint16_t m_samplerIndex = 0u;
        VmaAllocation m_memory = nullptr;
        VkImage m_image = VK_NULL_HANDLE;
        VulkanResourceView m_defaultView{};
    };
}

#pragma once
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Rendering/Structs/Descriptors.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace Systems;

    class VulkanTexture : public PK::Core::NoCopy
    {
        public:
            VulkanTexture(const VulkanDriver* driver, const char* filepath);
            VulkanTexture(const VulkanDriver* driver, const TextureDescriptor& descriptor);

            ~VulkanTexture();
            
            void SetData(const void* data, size_t size, uint32_t level, uint32_t layer) const;

            void Import(const char* filepath);

            const VulkanImageView* GetDefaultView() const { return m_imageViews.at({ m_defaultViewRange, false }).get(); }

            const VulkanImageView* GetAttachmentView() { return GetView(m_defaultViewRange, true); }

            const VulkanRenderTarget GetRenderTarget();

            const VulkanBindHandle* GetBindHandle() const;

            VkImageLayout GetImageLayout() const { return EnumConvert::GetImageLayout(m_descriptor.usage); }

            VkFormat GetNativeFormat() const { return m_rawImage->format; }

            const SamplerDescriptor& GetSamplerDescriptor() const { return m_descriptor.sampler; }

            bool Validate(const uint3 resolution);

            bool Validate(const TextureDescriptor& descriptor);

        private:
            struct ViewKey
            {
                VkImageSubresourceRange range;
                bool isAttachment;

                inline constexpr bool operator < (const ViewKey& other) const noexcept
                {
                    return range.aspectMask < other.range.aspectMask ||
                        range.baseMipLevel < other.range.baseMipLevel ||
                        range.levelCount < other.range.levelCount ||
                        range.baseArrayLayer < other.range.baseArrayLayer ||
                        range.layerCount < other.range.layerCount ||
                        isAttachment < other.isAttachment;
                }
            };

            const VulkanImageView* GetView(const VkImageSubresourceRange& range, bool isAttachment);
            
            void Rebuild(const TextureDescriptor& descriptor);

            void Dispose();

            const VulkanDriver* m_driver = nullptr;
            Ref<VulkanRawImage> m_rawImage = nullptr;
            Scope<VulkanBindHandle> m_bindHandle = nullptr;
            std::map<ViewKey, Ref<VulkanImageView>> m_imageViews;

            VkComponentMapping m_swizzle{};
            TextureDescriptor m_descriptor;
            VkImageSubresourceRange m_defaultViewRange;
            VkImageViewType m_viewType;
    };
}
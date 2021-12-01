#pragma once
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Rendering/Objects/Texture.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace Systems;
    using namespace PK::Rendering::Objects;

    class VulkanTexture : public Texture
    {
        public:
            VulkanTexture();
            VulkanTexture(const TextureDescriptor& descriptor);
            ~VulkanTexture();
            
            void SetData(const void* data, size_t size, uint32_t level, uint32_t layer) const override final;
            void Import(const char* filepath) override final;
            bool Validate(const uint3 resolution) override final;
            bool Validate(const TextureDescriptor& descriptor) override final;
            void Rebuild(const TextureDescriptor& descriptor);

            VkImageLayout GetImageLayout() const { return EnumConvert::GetImageLayout(m_descriptor.usage); }
            VkFormat GetNativeFormat() const { return m_rawImage->format; }
            const SamplerDescriptor& GetSamplerDescriptor() const { return m_descriptor.sampler; }
            const VulkanImageView* GetDefaultView() const { return m_imageViews.at({ m_defaultViewRange, false }).get(); }
            const VulkanImageView* GetAttachmentView() { return GetView(m_defaultViewRange, true); }
            const VulkanRenderTarget GetRenderTarget();
            const VulkanBindHandle* GetBindHandle() const;

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

            void Dispose();

            const VulkanDriver* m_driver = nullptr;
            Ref<VulkanRawImage> m_rawImage = nullptr;
            Scope<VulkanBindHandle> m_bindHandle = nullptr;
            std::map<ViewKey, Ref<VulkanImageView>> m_imageViews;
            VkComponentMapping m_swizzle{};
            VkImageSubresourceRange m_defaultViewRange;
            VkImageViewType m_viewType;
    };
}
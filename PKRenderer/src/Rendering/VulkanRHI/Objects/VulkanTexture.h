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

            inline VkImageLayout GetImageLayout() const { return EnumConvert::GetImageLayout(m_descriptor.usage); }
            inline VkImageAspectFlags GetAspectFlags() const { return m_rawImage->aspect; }
            inline VkFormat GetNativeFormat() const { return m_rawImage->format; }
            inline const VulkanRawImage* GetRaw() const { return m_rawImage; }
            const VulkanRenderTarget GetRenderTarget() const;
            const VulkanRenderTarget GetRenderTarget(uint32_t level, uint32_t levelCount, uint32_t layer, uint32_t layerCount);
            inline const VulkanBindHandle* GetBindHandle() const { return &GetView(m_defaultViewRange)->bindHandle; }
            inline const VulkanBindHandle* GetBindHandle(uint32_t level, uint32_t levelCount, uint32_t layer, uint32_t layerCount, bool sampled)
            {
                return &GetView(level, levelCount, layer, layerCount, sampled ? TextureBindMode::SampledTexture : TextureBindMode::Image)->bindHandle;
            }

        private:
            struct ViewKey
            {
                VkImageSubresourceRange range;
                TextureBindMode bindMode;

                inline constexpr bool operator < (const ViewKey& other) const noexcept
                {
                    return range.aspectMask < other.range.aspectMask ||
                        range.baseMipLevel < other.range.baseMipLevel ||
                        range.levelCount < other.range.levelCount ||
                        range.baseArrayLayer < other.range.baseArrayLayer ||
                        range.layerCount < other.range.layerCount ||
                        (int)bindMode < (int)other.bindMode;
                }
            };

            struct ViewValue
            {
                VulkanBindHandle bindHandle{};
                VulkanImageView* view = nullptr;
            };

            inline const ViewValue* GetView(const VkImageSubresourceRange& range, TextureBindMode mode = TextureBindMode::SampledTexture) const 
            {
                return m_imageViews.at({ range, mode }).get(); 
            }

            const ViewValue* GetView(uint32_t level, uint32_t levelCount, uint32_t layer, uint32_t layerCount, TextureBindMode mode = TextureBindMode::SampledTexture);
            const ViewValue* GetView(const VkImageSubresourceRange& range, TextureBindMode mode = TextureBindMode::SampledTexture);

            void Dispose();

            const VulkanDriver* m_driver = nullptr;
            VulkanRawImage* m_rawImage = nullptr;
            std::map<ViewKey, Scope<ViewValue>> m_imageViews;
            VkComponentMapping m_swizzle{};
            VkImageSubresourceRange m_defaultViewRange;
            VkImageViewType m_viewType;
            uint32_t m_version = 0u;
    };
}
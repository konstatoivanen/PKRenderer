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
            void SetSampler(const SamplerDescriptor& sampler) override final;
            void Import(const char* filepath) override final;
            bool Validate(const uint3& resolution) override final;
            bool Validate(const uint32_t levels, const uint32_t layers) override final;
            bool Validate(const TextureDescriptor& descriptor) override final;
            void Rebuild(const TextureDescriptor& descriptor);

            TextureViewRange NormalizeViewRange(const TextureViewRange& range) const;
            inline VkImageLayout GetImageLayout() const { return EnumConvert::GetImageLayout(m_descriptor.usage); }
            inline VkImageAspectFlags GetAspectFlags() const { return m_rawImage->aspect; }
            inline VkFormat GetNativeFormat() const { return m_rawImage->format; }
            inline const VulkanRawImage* GetRaw() const { return m_rawImage; }
            VulkanRenderTarget GetRenderTarget() const;
            VulkanRenderTarget GetRenderTarget(const TextureViewRange& range);
            inline const VulkanBindHandle* GetBindHandle() const { return &GetView(m_defaultViewRange)->bindHandle; }
            inline const VulkanBindHandle* GetBindHandle(const TextureViewRange& range, bool sampled) { return &GetView(range, sampled ? TextureBindMode::SampledTexture : TextureBindMode::Image)->bindHandle; }

        private:
            struct alignas(8) ViewKey
            {
                TextureViewRange range{};
                TextureBindMode bindMode = TextureBindMode::SampledTexture;

                inline constexpr bool operator < (const ViewKey& other) const noexcept
                {
                    if (range.level != other.range.level)
                    {
                        return range.level < other.range.level;
                    }

                    if (range.layer != other.range.layer)
                    {
                        return range.layer < other.range.layer;
                    }

                    if (range.levels != other.range.levels)
                    {
                        return range.levels < other.range.levels;
                    }

                    if (range.layers != other.range.layers)
                    {
                        return range.layers < other.range.layers;
                    }

                    if (bindMode != other.bindMode)
                    {
                        return (int)bindMode < (int)other.bindMode;
                    }

                    return false;
                }
            };

            struct ViewValue
            {
                VulkanBindHandle bindHandle{};
                VulkanImageView* view = nullptr;
            };

            inline const ViewValue* GetView(const TextureViewRange& range, TextureBindMode mode = TextureBindMode::SampledTexture) const
            {
                return m_imageViews.at({ range, mode }).get(); 
            }

            const ViewValue* GetView(const TextureViewRange& range, TextureBindMode mode = TextureBindMode::SampledTexture);

            void Dispose();

            const VulkanDriver* m_driver = nullptr;
            VulkanRawImage* m_rawImage = nullptr;
            std::map<ViewKey, Scope<ViewValue>> m_imageViews;
            VkComponentMapping m_swizzle{};
            TextureViewRange m_defaultViewRange{};
            VkImageViewType m_viewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    };
}
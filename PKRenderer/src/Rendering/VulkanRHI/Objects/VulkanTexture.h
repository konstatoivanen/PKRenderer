#pragma once
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Rendering/Objects/Texture.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    class VulkanTexture : public Rendering::Objects::Texture
    {
        public:
            VulkanTexture();
            VulkanTexture(const TextureDescriptor& descriptor, const char* name);
            ~VulkanTexture();
            
            void SetData(const void* data, size_t size, uint32_t level, uint32_t layer) const override final;
            void SetSampler(const Structs::SamplerDescriptor& sampler) override final;
            void Import(const char* filepath) override final;
            bool Validate(const Math::uint3& resolution) override final;
            bool Validate(const uint32_t levels, const uint32_t layers) override final;
            bool Validate(const Structs::TextureDescriptor& descriptor) override final;
            void Rebuild(const Structs::TextureDescriptor& descriptor);

            Structs::TextureViewRange NormalizeViewRange(const Structs::TextureViewRange& range) const;
            inline VkImageLayout GetImageLayout() const { return EnumConvert::GetImageLayout(m_descriptor.usage); }
            inline VkImageAspectFlags GetAspectFlags() const { return m_rawImage->aspect; }
            inline VkFormat GetNativeFormat() const { return m_rawImage->format; }
            inline const VulkanRawImage* GetRaw() const { return m_rawImage; }
            VulkanRenderTarget GetRenderTarget() const;
            VulkanRenderTarget GetRenderTarget(const Structs::TextureViewRange& range, bool includeView = true);
            inline const VulkanBindHandle* GetBindHandle() const { return &GetView(m_defaultViewRange)->bindHandle; }
            inline const VulkanBindHandle* GetBindHandle(const Structs::TextureViewRange& range, bool sampled) 
            { 
                return &GetView(range, sampled ? Structs::TextureBindMode::SampledTexture : Structs::TextureBindMode::Image)->bindHandle;
            }

        private:
            struct alignas(8) ViewKey
            {
                Structs::TextureViewRange range{};
                Structs::TextureBindMode bindMode = Structs::TextureBindMode::SampledTexture;

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

            inline const ViewValue* GetView(const Structs::TextureViewRange& range, Structs::TextureBindMode mode = Structs::TextureBindMode::SampledTexture) const
            {
                return m_imageViews.at({ range, mode }).get(); 
            }

            const ViewValue* GetView(const Structs::TextureViewRange& range, Structs::TextureBindMode mode = Structs::TextureBindMode::SampledTexture);

            void Dispose(const ExecutionGate& gate);

            const VulkanDriver* m_driver = nullptr;
            std::string m_name = "Texture";
            VulkanRawImage* m_rawImage = nullptr;
            std::map<ViewKey, PK::Utilities::Scope<ViewValue>> m_imageViews;
            VkComponentMapping m_swizzle{};
            Structs::TextureViewRange m_defaultViewRange{};
            VkImageViewType m_viewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    };
}
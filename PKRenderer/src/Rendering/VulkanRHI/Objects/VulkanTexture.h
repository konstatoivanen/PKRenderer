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
            
            void SetSampler(const Structs::SamplerDescriptor& sampler) final;
            bool Validate(const Math::uint3& resolution) final;
            bool Validate(const uint32_t levels, const uint32_t layers) final;
            bool Validate(const Structs::TextureDescriptor& descriptor) final;
            void Rebuild(const Structs::TextureDescriptor& descriptor);

            Structs::TextureViewRange NormalizeViewRange(const Structs::TextureViewRange& range) const;
            inline VkImageLayout GetImageLayout() const { return EnumConvert::GetImageLayout(m_descriptor.usage); }
            inline VkImageAspectFlags GetAspectFlags() const { return m_rawImage->aspect; }
            inline VkFormat GetNativeFormat() const { return m_rawImage->format; }
            inline const VulkanRawImage* GetRaw() const { return m_rawImage; }
            inline const VulkanBindHandle* GetBindHandle() const { return GetView(m_defaultViewRange)->bindHandle; }
            inline const VulkanBindHandle* GetBindHandle(TextureBindMode bindMode) { return GetView(m_defaultViewRange, bindMode)->bindHandle; }
            inline const VulkanBindHandle* GetBindHandle(const Structs::TextureViewRange& range, TextureBindMode bindMode) { return GetView(range, bindMode)->bindHandle; }
            void FillBindHandle(VulkanBindHandle* handle, const Structs::TextureViewRange& range, TextureBindMode bindMode) const;
            inline void FillBindHandle(VulkanBindHandle* handle, TextureBindMode bindMode) const { FillBindHandle(handle, m_defaultViewRange, bindMode); }
        
        private:
            inline static size_t GetViewKey(const Structs::TextureViewRange& range, Structs::TextureBindMode mode)
            {
                size_t h = 0ull;
                h |= range.level & 0xFFull;
                h |= (range.levels & 0xFFull) << 8ull;
                h |= (size_t)range.layer << 16ull;
                h |= (size_t)range.layers << 32ull;
                h |= (size_t)mode << 48ull;
                return h;
            }

            struct ViewValue
            {
                VulkanBindHandle* bindHandle = nullptr;
                VulkanImageView* view = nullptr;
            };

            inline const ViewValue* GetView(const Structs::TextureViewRange& range, Structs::TextureBindMode mode = Structs::TextureBindMode::SampledTexture) const
            {
                return m_imageViews.GetValueRef(GetViewKey(range, mode));
            }

            const ViewValue* GetView(const Structs::TextureViewRange& range, Structs::TextureBindMode mode = Structs::TextureBindMode::SampledTexture);

            void Dispose();

            const VulkanDriver* m_driver = nullptr;
            VulkanRawImage* m_rawImage = nullptr;
            PK::Utilities::FastMap<size_t, ViewValue> m_imageViews;
            VkComponentMapping m_swizzle{};
            Structs::TextureViewRange m_defaultViewRange{};
            VkImageViewType m_viewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    };
}
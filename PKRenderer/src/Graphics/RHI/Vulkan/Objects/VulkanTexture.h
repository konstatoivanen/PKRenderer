#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Utilities/FastMap.h"
#include "Graphics/RHI/RHITexture.h"
#include "Graphics/RHI/Vulkan/VulkanCommon.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Graphics::RHI::Vulkan, struct VulkanDriver)

namespace PK::Graphics::RHI::Vulkan::Objects
{
    class VulkanTexture : public RHITexture
    {
        public:
            VulkanTexture(const TextureDescriptor& descriptor, const char* name);
            ~VulkanTexture();
            
            void SetSampler(const SamplerDescriptor& sampler) final;
            bool Validate(const TextureDescriptor& descriptor) final;
            bool Validate(const uint32_t levels, const uint32_t layers) final;
            bool Validate(const Math::uint3& resolution) final;
            void Rebuild(const TextureDescriptor& descriptor);

            inline VkImageLayout GetImageLayout() const { return EnumConvert::GetImageLayout(m_descriptor.usage); }
            inline VkImageAspectFlags GetAspectFlags() const { return EnumConvert::GetFormatAspect(m_rawImage->format); }
            inline VkFormat GetNativeFormat() const { return m_rawImage->format; }
            inline const VulkanRawImage* GetRaw() const { return m_rawImage; }
            inline const VulkanBindHandle* GetBindHandle() { return GetView({})->bindHandle; }
            inline const VulkanBindHandle* GetBindHandle(TextureBindMode bindMode) { return GetView({}, bindMode)->bindHandle; }
            inline const VulkanBindHandle* GetBindHandle(const TextureViewRange& range, TextureBindMode bindMode) { return GetView(range, bindMode)->bindHandle; }
            void FillBindHandle(VulkanBindHandle* handle, const TextureViewRange& range, TextureBindMode bindMode) const;
            inline void FillBindHandle(VulkanBindHandle* handle, TextureBindMode bindMode) const { FillBindHandle(handle, {}, bindMode); }
        
        private:
            TextureViewRange NormalizeViewRange(const TextureViewRange& range) const;
            
            inline static size_t GetViewKey(const TextureViewRange& range, TextureBindMode mode)
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

            const ViewValue* GetView(const TextureViewRange& range, TextureBindMode mode = TextureBindMode::SampledTexture);

            void Dispose();

            const VulkanDriver* m_driver = nullptr;
            VulkanRawImage* m_rawImage = nullptr;
            PK::Utilities::FastMap<size_t, ViewValue> m_imageViews;
            VkComponentMapping m_swizzle{};
            VkImageViewType m_viewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    };
}
#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanDriver;

    class VulkanTexture : public RHITexture
    {
        public:
            VulkanTexture(const TextureDescriptor& descriptor, const char* name);
            ~VulkanTexture();
            
            void SetSampler(const SamplerDescriptor& sampler) final;
            const TextureDescriptor& GetDescriptor() const final { return m_descriptor; }
            const char* GetDebugName() const final { return m_name.c_str(); }
            void* GetNativeHandle() const final { return m_rawImage->image; }

            inline VkImageLayout GetImageLayout() const { return VulkanEnumConvert::GetImageLayout(m_descriptor.usage); }
            inline VkImageAspectFlags GetAspectFlags() const { return VulkanEnumConvert::GetFormatAspect(m_rawImage->format); }
            inline const VulkanBindHandle* GetBindHandle() { return &GetView({})->bindHandle; }
            inline const VulkanBindHandle* GetBindHandle(TextureBindMode bindMode) { return &GetView({}, bindMode)->bindHandle; }
            inline const VulkanBindHandle* GetBindHandle(const TextureViewRange& range, TextureBindMode bindMode) { return &GetView(range, bindMode)->bindHandle; }
            void FillBindHandle(VulkanBindHandle* handle, const TextureViewRange& range, TextureBindMode bindMode) const;
            inline void FillBindHandle(VulkanBindHandle* handle, TextureBindMode bindMode) const { FillBindHandle(handle, {}, bindMode); }
        
        private:
            TextureViewRange NormalizeViewRange(const TextureViewRange& range) const;
            
            inline static uint64_t GetViewKey(const TextureViewRange& range, TextureBindMode mode)
            {
                uint64_t h = 0ull;
                h |= range.level & 0xFFull;
                h |= (range.levels & 0xFFull) << 8ull;
                h |= (uint64_t)range.layer << 16ull;
                h |= (uint64_t)range.layers << 32ull;
                h |= (uint64_t)mode << 48ull;
                return h;
            }

            const VulkanImageView* GetView(const TextureViewRange& range, TextureBindMode mode = TextureBindMode::SampledTexture);

            const VulkanDriver* m_driver = nullptr;
            FixedString128 m_name;
            TextureDescriptor m_descriptor;
            VulkanRawImage* m_rawImage = nullptr;
            FastLinkedListRoot<VulkanImageView, uint64_t> m_firstView = nullptr;
    };
}
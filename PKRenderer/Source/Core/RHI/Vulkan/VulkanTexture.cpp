#include "PrecompiledHeader.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanTexture.h"

namespace PK
{
    VulkanTexture::VulkanTexture(struct VulkanDriver* driver, const TextureDescriptor& descriptor, const char* name) :
        m_driver(driver),
        m_name(name)
    {
        auto& families = m_driver->queues->GetSelectedFamilies();
        m_descriptor = descriptor;
        m_rawImage = m_driver->CreatePooled<VulkanRawImage>(m_driver->device, m_driver->allocator, VulkanImageCreateInfo(descriptor, &families), m_name.c_str());
    }

    VulkanTexture::~VulkanTexture()
    {
        auto fence = m_driver->GetQueues()->GetLastSubmitFenceRef();

        for (auto view : m_firstView)
        {
            m_driver->DisposePooled(view, fence);
        }

        if (m_rawImage != nullptr)
        {
            m_driver->DisposePooled(m_rawImage, fence);
            m_rawImage = nullptr;
        }

        m_firstView = nullptr;
    }

    void VulkanTexture::SetSampler(const SamplerDescriptor& sampler)
    {
        if (m_descriptor.sampler == sampler)
        {
            return;
        }

        m_descriptor.sampler = sampler;

        for (auto& view : m_firstView)
        {
            if (view->bindHandle.image.sampler != VK_NULL_HANDLE)
            {
                view->bindHandle.IncrementVersion();
                view->bindHandle.image.sampler = m_driver->samplerCache->GetSampler(m_descriptor.sampler);
            }
        }
    }

    TextureViewRange VulkanTexture::NormalizeViewRange(const TextureViewRange& range) const
    {
        auto out = range;
        auto levels = (uint16_t)m_rawImage->levels;
        auto layers = (uint16_t)m_rawImage->layers;
        auto viewType = VulkanEnumConvert::GetViewType(m_descriptor.type);

        // Remove layers from non layered image types
        switch (viewType)
        {
            case VK_IMAGE_VIEW_TYPE_1D:
            case VK_IMAGE_VIEW_TYPE_2D:
            case VK_IMAGE_VIEW_TYPE_3D: out.layers = 1u; break;
            case VK_IMAGE_VIEW_TYPE_CUBE: out.layers = 6u; break;
            default: break;
        }

        if (out.level >= levels) out.level = levels - 1u;
        if (out.layer >= layers) out.layer = layers - 1u;
        if (out.levels == 0 || out.level + out.levels >= levels) out.levels = 0x7FFF;
        if (out.layers == 0 || out.layer + out.layers >= layers) out.layers = 0x7FFF;

        // Use clamped range for render targets as framebuffers cannot have unbounded layer ranges
        // Not checked based on bind type because of access tracking 
        if ((m_descriptor.usage & TextureUsage::ValidRTTypes) != 0)
        {
            if (out.levels == 0x7FFF) out.levels = glm::max(1, levels - out.level);
            if (out.layers == 0x7FFF) out.layers = glm::max(1, layers - out.layer);
        }

        return out;
    }

    void VulkanTexture::FillBindHandle(VulkanBindHandle* handle, const TextureViewRange& range, TextureBindMode bindMode) const
    {
        auto normalizedRange = NormalizeViewRange(range);
        handle->isConcurrent = IsConcurrent();
        handle->isTracked = IsTracked();
        handle->image.view = VK_NULL_HANDLE;
        handle->image.image = m_rawImage->image;
        handle->image.alias = m_rawImage->imageAlias;
        handle->image.layout = GetImageLayout();
        handle->image.format = m_rawImage->format;
        handle->image.extent = m_rawImage->extent;
        handle->image.samples = m_rawImage->samples;
        handle->image.range =
        {
            (uint32_t)VulkanEnumConvert::GetFormatAspect(handle->image.format),
            normalizedRange.level,
            VulkanEnumConvert::ExpandVkRange16(normalizedRange.levels),
            normalizedRange.layer,
            VulkanEnumConvert::ExpandVkRange16(normalizedRange.layers)
        };

        if (bindMode == TextureBindMode::SampledTexture)
        {
            handle->image.sampler = m_driver->samplerCache->GetSampler(m_descriptor.sampler);
        }
    }

    const VulkanImageView* VulkanTexture::GetView(const TextureViewRange& range, TextureBindMode mode)
    {
        auto normalizedRange = NormalizeViewRange(range);
        auto key = GetViewKey(normalizedRange, mode);

        if (m_firstView.FindAndSwapFirst(key))
        {
            return m_firstView;
        }

        auto useAlias = mode != TextureBindMode::SampledTexture && m_rawImage->imageAlias != VK_NULL_HANDLE;
        auto viewType = VulkanEnumConvert::GetViewType(m_descriptor.type);
        auto swizzle = VulkanEnumConvert::GetSwizzle(m_rawImage->format);

        VulkanImageViewCreateInfo info;
        info.image = m_rawImage->image;
        info.imageAlias = m_rawImage->imageAlias;
        info.viewType = viewType;
        info.format = m_rawImage->format;
        info.formatAlias = m_rawImage->formatAlias;
        info.layout = GetImageLayout();
        info.samples = m_rawImage->samples;
        info.components = mode == TextureBindMode::SampledTexture ? swizzle : (VkComponentMapping{});
        info.extent = m_rawImage->extent;
        info.isConcurrent = IsConcurrent();
        info.isTracked = IsTracked();
        info.isAlias = useAlias;
        info.subresourceRange =
        {
            (uint32_t)VulkanEnumConvert::GetFormatAspect(info.format),
            normalizedRange.level,
            VulkanEnumConvert::ExpandVkRange16(normalizedRange.levels),
            normalizedRange.layer,
            VulkanEnumConvert::ExpandVkRange16(normalizedRange.layers)
        };

        auto newView = m_driver->CreatePooled<VulkanImageView>(m_driver->device, info, m_name.c_str());

        if (mode == TextureBindMode::SampledTexture)
        {
            newView->bindHandle.image.sampler = m_driver->samplerCache->GetSampler(m_descriptor.sampler);
        }

        m_firstView.Insert(newView, key);
        return m_firstView;
    }
}

#include "PrecompiledHeader.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanTexture.h"

namespace PK
{
    VulkanTexture::VulkanTexture(const TextureDescriptor& descriptor, const char* name) :  
        m_driver(RHIDriver::Get()->GetNative<VulkanDriver>()),
        m_name(name)
    {
        auto& families = m_driver->queues->GetSelectedFamilies();
        m_descriptor = descriptor;
        m_rawImage = m_driver->imagePool.New(m_driver->device, m_driver->allocator, VulkanImageCreateInfo(descriptor, &families), m_name.c_str());
    }

    VulkanTexture::~VulkanTexture()
    {
        auto fence = m_driver->GetQueues()->GetFenceRef(QueueType::Graphics);

        for (auto i = 0u; i < m_imageViews.GetCount(); ++i)
        {
            auto value = &m_imageViews.GetValueAt(i);
            m_driver->bindhandlePool.Delete(value->bindHandle);
            m_driver->DisposePooledImageView(value->view, fence);
        }

        m_imageViews.Clear();

        if (m_rawImage != nullptr)
        {
            m_driver->DisposePooledImage(m_rawImage, fence);
            m_rawImage = nullptr;
        }
    }

    void VulkanTexture::SetSampler(const SamplerDescriptor& sampler)
    {
        if (m_descriptor.sampler == sampler)
        {
            return;
        }

        m_descriptor.sampler = sampler;

        for (auto i = 0u; i < m_imageViews.GetCount(); ++i)
        {
            auto value = &m_imageViews.GetValueAt(i);

            if (value->bindHandle->image.sampler != VK_NULL_HANDLE)
            {
                value->bindHandle->IncrementVersion();
                value->bindHandle->image.sampler = RHIDriver::Get()->GetNative<VulkanDriver>()->samplerCache->GetSampler(m_descriptor.sampler);
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
            handle->image.sampler = RHIDriver::Get()->GetNative<VulkanDriver>()->samplerCache->GetSampler(m_descriptor.sampler);
        }
    }

    const VulkanTexture::ViewValue* VulkanTexture::GetView(const TextureViewRange& range, TextureBindMode mode)
    {
        auto normalizedRange = NormalizeViewRange(range);
        size_t key = GetViewKey(normalizedRange, mode);
        auto index = 0u;

        if (!m_imageViews.AddKey(key, &index))
        {
            return &m_imageViews.GetValueAt(index);
        }

        auto useAlias = mode != TextureBindMode::SampledTexture && m_rawImage->imageAlias != VK_NULL_HANDLE;
        auto viewType = VulkanEnumConvert::GetViewType(m_descriptor.type);
        auto swizzle = VulkanEnumConvert::GetSwizzle(m_rawImage->format);

        VkImageViewCreateInfo info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        info.pNext = nullptr;
        info.flags = 0;
        info.image = useAlias ? m_rawImage->imageAlias : m_rawImage->image;
        info.viewType = viewType;
        info.format = useAlias ? m_rawImage->formatAlias : m_rawImage->format;
        info.components = mode == TextureBindMode::SampledTexture ? swizzle : (VkComponentMapping{});
        info.subresourceRange =
        {
            (uint32_t)VulkanEnumConvert::GetFormatAspect(info.format),
            normalizedRange.level,
            VulkanEnumConvert::ExpandVkRange16(normalizedRange.levels),
            normalizedRange.layer,
            VulkanEnumConvert::ExpandVkRange16(normalizedRange.layers)
        };

        auto viewValue = &m_imageViews.GetValueAt(index);
        viewValue->view = m_driver->imageViewPool.New(m_driver->device, info, m_name.c_str());
        viewValue->bindHandle = m_driver->bindhandlePool.New();
        viewValue->bindHandle->image.view = viewValue->view->view;
        viewValue->bindHandle->image.image = m_rawImage->image;
        viewValue->bindHandle->image.alias = m_rawImage->imageAlias;
        viewValue->bindHandle->image.layout = GetImageLayout();
        viewValue->bindHandle->image.format = info.format;
        viewValue->bindHandle->image.extent = m_rawImage->extent;
        viewValue->bindHandle->image.range = info.subresourceRange;
        viewValue->bindHandle->image.samples = m_rawImage->samples;
        viewValue->bindHandle->isConcurrent = IsConcurrent();
        viewValue->bindHandle->isTracked = IsTracked();

        if (mode == TextureBindMode::SampledTexture)
        {
            viewValue->bindHandle->image.sampler = RHIDriver::Get()->GetNative<VulkanDriver>()->samplerCache->GetSampler(m_descriptor.sampler);
        }

        return viewValue;
    }
}
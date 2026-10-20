#include "PrecompiledHeader.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanTexture.h"

namespace PK
{
    VulkanTexture::VulkanTexture(struct VulkanDriver* driver, const TextureDescriptor& descriptor, const char* name) :
        m_name(name),
        m_driver(driver),
        m_format(VulkanEnumConvert::GetFormat(descriptor.format)),
        m_formatAlias(VulkanEnumConvert::GetFormat(descriptor.formatAlias)),
        m_descriptor(descriptor)
    {
        auto& familiesAll = m_driver->queues->GetFamiliesAll();
        auto& familiesRender = m_driver->queues->GetFamiliesRender();
        const auto requiresTransfer = (descriptor.usage & TextureUsage::Upload) != 0;

        VkFormat viewFormats[2] = { m_format, m_formatAlias };
        VkImageFormatListCreateInfo formatListInfo{ VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO };
        formatListInfo.viewFormatCount = 2u;
        formatListInfo.pViewFormats = viewFormats;

        VkImageCreateInfo imageCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageCreateInfo.flags = 0u;
        imageCreateInfo.imageType = descriptor.type == TextureType::Texture3D ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = m_format;
        imageCreateInfo.extent = { descriptor.resolution.x, descriptor.resolution.y, descriptor.resolution.z };
        imageCreateInfo.mipLevels = descriptor.levels;
        imageCreateInfo.arrayLayers = descriptor.layers;
        imageCreateInfo.samples = VulkanEnumConvert::GetSampleCountFlags(descriptor.samples);
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = 0;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
        imageCreateInfo.pQueueFamilyIndices = requiresTransfer ? familiesAll.indices : familiesRender.indices;
        imageCreateInfo.queueFamilyIndexCount = requiresTransfer ? familiesAll.count : familiesRender.count;

        if (descriptor.type != TextureType::Texture3D)
        {
            imageCreateInfo.extent.depth = 1u;
        }

        if (descriptor.type == TextureType::Cubemap || descriptor.type == TextureType::CubemapArray)
        {
            imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        if (m_formatAlias != VK_FORMAT_UNDEFINED && m_formatAlias != m_format)
        {
            imageCreateInfo.flags |= VK_IMAGE_CREATE_EXTENDED_USAGE_BIT | VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
            imageCreateInfo.pNext = &formatListInfo;
        }

        if ((descriptor.usage & TextureUsage::RTColor) != 0)   imageCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if ((descriptor.usage & TextureUsage::RTDepth) != 0)   imageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if ((descriptor.usage & TextureUsage::RTStencil) != 0) imageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        if ((descriptor.usage & TextureUsage::Upload) != 0)    imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        if ((descriptor.usage & TextureUsage::Sample) != 0)    imageCreateInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        if ((descriptor.usage & TextureUsage::Input) != 0)     imageCreateInfo.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        if ((descriptor.usage & TextureUsage::Storage) != 0)   imageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        
        VmaAllocationCreateInfo allocationCreateInfo{};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;

        if ((descriptor.usage & TextureUsage::Transient) != 0)
        {
            imageCreateInfo.usage &= ~(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
            allocationCreateInfo.preferredFlags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
        }

        VK_ASSERT_RESULT_CTX(vmaCreateImage(m_driver->allocator, &imageCreateInfo, &allocationCreateInfo, &m_image, &m_memory, nullptr), "Failed to create a VkImage!");
        VulkanSetObjectDebugName(m_driver->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)m_image, name);

        m_samplerIndex = m_driver->descriptorHeap->GetSamplerIndex(m_descriptor.sampler);
        m_defaultView = GetResourceView({}, TextureViewMode::SRV);
        m_driver->graph->RegisterImage(m_image, VulkanEnumConvert::GetFormatAspect(m_format));
    }

    VulkanTexture::~VulkanTexture()
    {
        auto fence = m_driver->graph->GetFenceRef();

        m_driver->disposer->Dispose(m_driver->device, m_image, [](void* c, void* v)
        {
            vkDestroyImage(static_cast<VkDevice>(c), static_cast<VkImage>(v), nullptr);
        },
        fence);

        m_driver->disposer->Dispose(m_driver->allocator, m_memory, [](void* c, void* v)
        {
            vmaFreeMemory(static_cast<VmaAllocator>(c), static_cast<VmaAllocation>(v));
        },
        fence);

        m_driver->graph->UnregisterImage(m_image);
    }

    void VulkanTexture::SetSampler(const SamplerDescriptor& sampler)
    {
        if (m_descriptor.sampler != sampler)
        {
            m_descriptor.sampler = sampler;
            m_samplerIndex = m_driver->descriptorHeap->GetSamplerIndex(m_descriptor.sampler);
            m_defaultView.image.samplerIndex = m_samplerIndex;
            m_defaultView.UpdateHashImage(Version());
        }
    }

    VulkanResourceView VulkanTexture::GetResourceView(const TextureViewRange& range, TextureViewMode mode) const
    {
        const auto normalizedRange = NormalizeViewRange(range);
        const auto useAlias = mode == TextureViewMode::UAV && m_formatAlias != VK_FORMAT_UNDEFINED;

        VulkanResourceView view{};
        view.image.image = m_image;
        view.image.format = useAlias ? m_formatAlias : m_format;
        view.image.viewType = VulkanEnumConvert::GetViewType(m_descriptor.type);
        view.image.samplerIndex = mode == TextureViewMode::SRV ? m_samplerIndex : 0u;
        view.image.samples = (uint16_t)VulkanEnumConvert::GetSampleCountFlags(m_descriptor.samples);
        view.image.extent = { m_descriptor.resolution.x, m_descriptor.resolution.y, m_descriptor.resolution.z };
        view.image.range = 
        {
            (uint32_t)VulkanEnumConvert::GetFormatAspect(view.image.format), // aspect
            normalizedRange.level,  //baseMipLevel
            normalizedRange.levels >= PK_VK_IMAGE_RANGE_MAX ? VK_REMAINING_MIP_LEVELS : normalizedRange.levels, //levelCount
            normalizedRange.layer,  //baseArrayLayer
            normalizedRange.layers >= PK_VK_IMAGE_RANGE_MAX ? VK_REMAINING_ARRAY_LAYERS : normalizedRange.layers //layerCount
        };

        if (mode != TextureViewMode::RAW)
        {
            view.UpdateHashImage(Version());
        }

        return view;
    }

    TextureViewRange VulkanTexture::NormalizeViewRange(const TextureViewRange& range) const
    {
        auto out = range;
        auto levels = (uint16_t)m_descriptor.levels;
        auto layers = (uint16_t)m_descriptor.layers;
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
        if (out.levels == 0 || out.level + out.levels >= levels) out.levels = PK_VK_IMAGE_RANGE_MAX;
        if (out.layers == 0 || out.layer + out.layers >= layers) out.layers = PK_VK_IMAGE_RANGE_MAX;

        // Use clamped range for render targets as framebuffers cannot have unbounded layer ranges
        // Not checked based on bind type because of access tracking is not aware of it. 
        if ((m_descriptor.usage & TextureUsage::ValidRTTypes) != 0)
        {
            if (out.levels == PK_VK_IMAGE_RANGE_MAX) out.levels = levels - out.level;
            if (out.layers == PK_VK_IMAGE_RANGE_MAX) out.layers = layers - out.layer;
        }

        return out;
    }
}

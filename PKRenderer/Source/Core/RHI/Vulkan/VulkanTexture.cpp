#include "PrecompiledHeader.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanTexture.h"

namespace PK
{
    constexpr static uint64_t GetViewKey(const TextureViewRange& range, TextureBindMode mode)
    {
        uint64_t h = 0ull;
        h |= range.level & 0xFFull;
        h |= (range.levels & 0xFFull) << 8ull;
        h |= (uint64_t)range.layer << 16ull;
        h |= (uint64_t)range.layers << 32ull;
        h |= (uint64_t)mode << 48ull;
        return h;
    }

    VulkanTexture::VulkanTexture(struct VulkanDriver* driver, const TextureDescriptor& descriptor, const char* name) :
        m_name(name),
        m_driver(driver),
        m_format(VulkanEnumConvert::GetFormat(descriptor.format)),
        m_formatAlias(VulkanEnumConvert::GetFormat(descriptor.formatAlias)),
        m_descriptor(descriptor)
    {
        auto& families = m_driver->queues->GetSelectedFamilies();

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
        imageCreateInfo.sharingMode = (descriptor.usage & TextureUsage::Concurrent) != 0 ? VK_SHARING_MODE_CONCURRENT : VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.pQueueFamilyIndices = families.indices;
        imageCreateInfo.queueFamilyIndexCount = families.count;

        VmaAllocationCreateInfo allocationCreateInfo{};
        allocationCreateInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        if (descriptor.type == TextureType::Cubemap ||
            descriptor.type == TextureType::CubemapArray)
        {
            imageCreateInfo.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        }

        if (descriptor.formatAlias != TextureFormat::Invalid && descriptor.formatAlias != descriptor.format)
        {
            // Set VK_IMAGE_CREATE_ALIAS_BIT  for block compressed formats
            imageCreateInfo.flags |= VK_IMAGE_CREATE_EXTENDED_USAGE_BIT | VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_ALIAS_BIT;
            allocationCreateInfo.flags |= VMA_ALLOCATION_CREATE_CAN_ALIAS_BIT;
        }

        if ((descriptor.usage & TextureUsage::RTColor) != 0)
        {
            imageCreateInfo.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if ((descriptor.usage & TextureUsage::RTDepth) != 0)
        {
            imageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if ((descriptor.usage & TextureUsage::RTStencil) != 0)
        {
            imageCreateInfo.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }

        if ((descriptor.usage & TextureUsage::Upload) != 0)
        {
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if ((descriptor.usage & TextureUsage::Sample) != 0)
        {
            imageCreateInfo.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }

        if ((descriptor.usage & TextureUsage::Input) != 0)
        {
            imageCreateInfo.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        }

        if ((descriptor.usage & TextureUsage::Storage) != 0)
        {
            imageCreateInfo.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        if ((descriptor.usage & TextureUsage::Transient) != 0)
        {
            imageCreateInfo.usage &= ~(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
            imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
            // Not supported on desktop
            // allocation.usage = VMA_MEMORY_USAGE_GPU_LAZILY_ALLOCATED;
        }

        if (imageCreateInfo.flags & VK_IMAGE_CREATE_ALIAS_BIT)
        {
            auto aliasInfo = imageCreateInfo;
            aliasInfo.usage &= ~VK_IMAGE_USAGE_STORAGE_BIT;
            VK_ASSERT_RESULT_CTX(vmaCreateImage(m_driver->allocator, &aliasInfo, &allocationCreateInfo, &m_image, &m_memory, nullptr), "Failed to create an image!");
            vmaCreateAliasingImage(m_driver->allocator, m_memory, &imageCreateInfo, &m_imageAlias);
            VulkanSetObjectDebugName(m_driver->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)m_imageAlias, FixedString128({ name, ".Alias" }).c_str());
        }
        else
        {
            m_imageAlias = VK_NULL_HANDLE;
            VK_ASSERT_RESULT_CTX(vmaCreateImage(m_driver->allocator, &imageCreateInfo, &allocationCreateInfo, &m_image, &m_memory, nullptr), "Failed to create an image!");
        }

        VulkanSetObjectDebugName(m_driver->device, VK_OBJECT_TYPE_IMAGE, (uint64_t)m_image, name);
    }

    VulkanTexture::~VulkanTexture()
    {
        auto fence = m_driver->GetQueues()->GetLastSubmitFenceRef();

        for (auto view : m_firstView)
        {
            m_driver->DisposePooled(view, fence);
        }

        if (m_imageAlias != VK_NULL_HANDLE)
        {
            m_driver->disposer->Dispose(m_driver->device, m_imageAlias, [](void* c, void* v)
            {
                vkDestroyImage(static_cast<VkDevice>(c), static_cast<VkImage>(v), nullptr);
            },
            fence);
        }

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

        m_firstView = nullptr;
    }

    void VulkanTexture::SetSampler(const SamplerDescriptor& sampler)
    {
        if (m_descriptor.sampler != sampler)
        {
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

    void VulkanTexture::FillBindHandle(VulkanBindHandle* handle, const TextureViewRange& range, TextureBindMode bindMode) const
    {
        auto normalizedRange = NormalizeViewRange(range);
        handle->isConcurrent = IsConcurrent();
        handle->isTracked = IsTracked();
        handle->image.view = VK_NULL_HANDLE;
        handle->image.image = m_image;
        handle->image.alias = m_imageAlias;
        handle->image.layout = GetImageLayout();
        handle->image.format = m_format;
        handle->image.extent = { m_descriptor.resolution.x, m_descriptor.resolution.y, m_descriptor.resolution.z };
        handle->image.samples = (uint16_t)VulkanEnumConvert::GetSampleCountFlags(m_descriptor.samples);
        handle->image.range = VulkanConvertRange(normalizedRange, handle->image.format);
 
        if (bindMode == TextureBindMode::SampledTexture)
        {
            handle->image.sampler = m_driver->samplerCache->GetSampler(m_descriptor.sampler);
        }
    }

    const VulkanImageView* VulkanTexture::GetView(const TextureViewRange& range, TextureBindMode mode)
    {
        auto normalizedRange = NormalizeViewRange(range);
        auto key = GetViewKey(normalizedRange, mode);

        if (!m_firstView.FindAndSwapFirst(key))
        {
            auto useAlias = mode != TextureBindMode::SampledTexture && m_imageAlias != VK_NULL_HANDLE;
            auto viewType = VulkanEnumConvert::GetViewType(m_descriptor.type);
            auto swizzle = VulkanEnumConvert::GetSwizzle(m_format);

            VulkanImageViewCreateInfo info;
            info.image = m_image;
            info.imageAlias = m_imageAlias;
            info.viewType = viewType;
            info.format = m_format;
            info.formatAlias = m_formatAlias;
            info.layout = GetImageLayout();
            info.samples = VulkanEnumConvert::GetSampleCountFlags(m_descriptor.samples);
            info.components = mode == TextureBindMode::SampledTexture ? swizzle : (VkComponentMapping{});
            info.extent = { m_descriptor.resolution.x, m_descriptor.resolution.y, m_descriptor.resolution.z };
            info.isConcurrent = IsConcurrent();
            info.isTracked = IsTracked();
            info.isAlias = useAlias;
            info.subresourceRange = VulkanConvertRange(normalizedRange, info.format);

            auto newView = m_driver->CreatePooled<VulkanImageView>(m_driver->device, info, m_name.c_str());

            if (mode == TextureBindMode::SampledTexture)
            {
                newView->bindHandle.image.sampler = m_driver->samplerCache->GetSampler(m_descriptor.sampler);
            }

            m_firstView.Insert(newView, key);
        }

        return m_firstView;
    }
}

#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "VulkanDescriptorHeap.h"

namespace PK
{
    VulkanDescriptorHeap::VulkanDescriptorHeap(VkDevice device,
        VmaAllocator allocator,
        const VulkanQueueFamilies& queueFamilies,
        const VulkanPhysicalDeviceProperties& properties,
        size_t resourceArenaSize,
        uint64_t pruneDelay) :
        m_device(device),
        m_allocator(allocator),
        m_properties(properties.descriptorHeapDerived),
        m_pruneDelay(pruneDelay)
    {
        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        allocInfo.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        allocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

        // Create resource heap
        {
            const auto descriptorsSize = PK_VK_MAX_DESCRIPTORS * m_properties.maxResourceStride;
            const auto reservedOffset = math::align(descriptorsSize, m_properties.resourceHeapOffset);
            const auto reservedSize = properties.descriptorHeap.minResourceHeapReservedRange;

            VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferInfo.size = math::align(reservedOffset + reservedSize, m_properties.resourceHeapAlignment);
            bufferInfo.usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = queueFamilies.count;
            bufferInfo.pQueueFamilyIndices = queueFamilies.indices;
            PK_FATAL_ASSERT(bufferInfo.size <= m_properties.maxResourceHeapSize, "heap size '%llu' exceeds allowed max size '%llu'!", bufferInfo.size, m_properties.maxResourceHeapSize);

            VmaAllocationInfo allocationInfo{};
            VK_ASSERT_RESULT_CTX(vmaCreateBufferWithAlignment(m_allocator, &bufferInfo, &allocInfo, m_properties.resourceHeapAlignment, &m_resourceHeap.buffer, &m_resourceHeap.memory, &allocationInfo), "Failed to create a resource heap buffer!");
            VulkanSetObjectDebugName(m_device, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_resourceHeap.buffer, "PK_Resource_Heap");
            m_resourceHeap.mappedPtr = static_cast<uint8_t*>(allocationInfo.pMappedData);

            m_resourceHeap.bindInfo.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT;
            m_resourceHeap.bindInfo.heapRange = { VulkanGetBudderDeviceAddress(m_device, m_resourceHeap.buffer), bufferInfo.size };
            m_resourceHeap.bindInfo.reservedRangeOffset = reservedOffset;
            m_resourceHeap.bindInfo.reservedRangeSize = reservedSize;
        }

        // Create sampler heap
        {
            const auto samplersSize = PK_VK_MAX_SAMPLERS * m_properties.samplerStride;
            const auto reservedOffset = math::align(samplersSize, m_properties.samplerHeapOffset);
            const auto reservedSize = properties.descriptorHeap.minSamplerHeapReservedRange;

            VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferInfo.size = math::align(reservedOffset + reservedSize, m_properties.samplerHeapAlignment);
            bufferInfo.usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = queueFamilies.count;
            bufferInfo.pQueueFamilyIndices = queueFamilies.indices;
            PK_FATAL_ASSERT(bufferInfo.size <= m_properties.maxSamplerHeapSize, "heap size '%llu' exceeds allowed max size '%llu'!", bufferInfo.size, m_properties.maxSamplerHeapSize);

            VmaAllocationInfo allocationInfo{};
            VK_ASSERT_RESULT_CTX(vmaCreateBufferWithAlignment(m_allocator, &bufferInfo, &allocInfo, m_properties.samplerHeapAlignment, &m_samplerHeap.buffer, &m_samplerHeap.memory, &allocationInfo), "Failed to create a sampler heap buffer!");
            VulkanSetObjectDebugName(m_device, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_samplerHeap.buffer, "PK_Sampler_Heap");
            m_samplerHeap.mappedPtr = static_cast<uint8_t*>(allocationInfo.pMappedData);

            m_samplerHeap.bindInfo.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT;
            m_samplerHeap.bindInfo.heapRange = { VulkanGetBudderDeviceAddress(m_device, m_samplerHeap.buffer), bufferInfo.size };
            m_samplerHeap.bindInfo.reservedRangeOffset = reservedOffset;
            m_samplerHeap.bindInfo.reservedRangeSize = reservedSize;
        }

        GetSamplerIndex(SamplerDescriptor{});
    }

    VulkanDescriptorHeap::~VulkanDescriptorHeap()
    {
        for (auto i = 0u; i < m_renderTargetViews.GetCount(); ++i)
        {
            vkDestroyImageView(m_device, m_renderTargetViews[i].value.view, nullptr);
        }

        vmaDestroyBuffer(m_allocator, m_resourceHeap.buffer, m_resourceHeap.memory);
        vmaDestroyBuffer(m_allocator, m_samplerHeap.buffer, m_samplerHeap.memory);
    }

    void VulkanDescriptorHeap::Prune()
    {
        m_currentPruneTick++;

        for (auto i = m_resourceCache.GetCount(); i > 0u; --i)
        {
            const auto& entry = m_resourceCache[i - 1].value;
            
            if (entry.pruneTick < m_currentPruneTick)
            {
                m_resourcePool.Delete(entry.index);
                m_resourceCache.RemoveAt(i - 1);
            }
        }

        for (auto i = m_renderTargetViews.GetCount(); i > 0u; --i)
        {
            const auto& entry = m_renderTargetViews[i - 1].value;

            if (entry.pruneTick < m_currentPruneTick)
            {
                vkDestroyImageView(m_device, entry.view, nullptr);
                m_renderTargetViews.RemoveAt(i - 1);
            }
        }
    }


    VkImageView VulkanDescriptorHeap::GetRenderTargetView(const VulkanResourceView& view)
    {
        auto index = 0u;

        if (m_renderTargetViews.AddKey(view.hash, &index))
        {
            VkImageViewUsageCreateInfo usageInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO };
            usageInfo.usage = VulkanEnumConvert::IsDepthFormat(view.image.format) ? 
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            VkImageViewCreateInfo info{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            info.pNext = &usageInfo;
            info.image = view.image.image;
            info.viewType = view.image.viewType;
            info.format = view.image.format;
            info.subresourceRange = view.image.range;

            VkImageView imageView = VK_NULL_HANDLE;
            VK_ASSERT_RESULT_CTX(vkCreateImageView(m_device, &info, nullptr, &imageView), "Failed to create RTV image view!");

            m_renderTargetViews[index].value.view = imageView; 
        }

        m_renderTargetViews[index].value.pruneTick = m_currentPruneTick + m_pruneDelay;
        return m_renderTargetViews[index].value.view;
    }

    uint16_t VulkanDescriptorHeap::GetSamplerIndex(const SamplerDescriptor& descriptor)
    {
        auto index = 0u;

        if (m_samplerCache.AddKey(descriptor, &index))
        {
            VkSamplerCreateInfo info{ VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
            info.addressModeU = VulkanEnumConvert::GetSamplerAddressMode(descriptor.wrap[0]);
            info.addressModeV = VulkanEnumConvert::GetSamplerAddressMode(descriptor.wrap[1]);
            info.addressModeW = VulkanEnumConvert::GetSamplerAddressMode(descriptor.wrap[2]);
            info.minLod = descriptor.mipMin;
            info.maxLod = descriptor.mipMax <= 0.0f ? VK_LOD_CLAMP_NONE : descriptor.mipMax;
            info.mipLodBias = descriptor.mipBias;
            info.maxAnisotropy = descriptor.anisotropy;
            info.anisotropyEnable = descriptor.anisotropy > 0.0f ? VK_TRUE : VK_FALSE;
            info.unnormalizedCoordinates = !descriptor.normalized;
            info.borderColor = VulkanEnumConvert::GetBorderColor(descriptor.borderColor);
            info.mipmapMode = (uint32_t)descriptor.filterMin > (uint32_t)FilterMode::Bilinear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
            info.compareEnable = descriptor.comparison != Comparison::Off ? VK_TRUE : VK_FALSE;
            info.compareOp = VulkanEnumConvert::GetCompareOp(descriptor.comparison);
            info.magFilter = VulkanEnumConvert::GetFilterMode(descriptor.filterMag);
            info.minFilter = VulkanEnumConvert::GetFilterMode(descriptor.filterMin);

            if (info.unnormalizedCoordinates)
            {
                info.minLod = info.maxLod = 0.0f;
            }

            auto* cpuAddress = m_samplerCache[index].value.data;
            auto* gpuAddress = m_samplerHeap.mappedPtr + index * m_properties.samplerStride;

            VkHostAddressRangeEXT hostRange{ cpuAddress, PK_VK_MAX_SAMPLER_DESCRIPTOR_SIZE };
            VK_ASSERT_RESULT_CTX(vkWriteSamplerDescriptorsEXT(m_device, 1u, &info, &hostRange), "Failed to write sampler descriptor!");

            memcpy(gpuAddress, cpuAddress, m_properties.samplerStride);
        }

        return index;
    }

    VulkanResourceView VulkanDescriptorHeap::GetSamplerView(const SamplerDescriptor& descriptor)
    {
        VulkanResourceView view{};
        view.samplerIndex = GetSamplerIndex(descriptor);
        view.hash = view.samplerIndex;
        return view;
    }

    uint32_t VulkanDescriptorHeap::GetBindlessIndex(const VulkanResourceView& view, ElementType type)
    {
        if (type == ElementType::Sampler)
        {
            return view.samplerIndex;
        }

        return GetOrCacheResourceView(view, type);
    }

    uint3 VulkanDescriptorHeap::GetTextureSize(uint32_t bindlessIndex) const
    {
        return m_resourcePool[bindlessIndex]->resolution;
    }

    uint32_t VulkanDescriptorHeap::GetOrCacheResourceView(const VulkanResourceView& view, ElementType type)
    {
        // Pure resource descriptor: SamplerTexture in legacy shaders uses the exact same SAMPLED_IMAGE resource descriptor as Texture
        const auto key = Hash::Mix(view.hash, (uint64_t)type);
        auto mapIndex = 0u;

        if (m_resourceCache.AddKey(key, &mapIndex))
        {
            auto entry = m_resourcePool.New();
            auto index = m_resourcePool.GetIndex(entry);
            m_resourceCache[mapIndex].value.index = index;

            VkImageViewUsageCreateInfo usageInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO };
            VkImageViewCreateInfo viewInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
            VkImageDescriptorInfoEXT imageInfo{ VK_STRUCTURE_TYPE_IMAGE_DESCRIPTOR_INFO_EXT };
            VkResourceDescriptorInfoEXT resourceInfo{ VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT };

            resourceInfo.type = VulkanEnumConvert::GetDescriptorType(type);

            if (type == ElementType::Texture || type == ElementType::Image)
            {
                viewInfo.pNext = &usageInfo;
                imageInfo.pView = &viewInfo;
                resourceInfo.data.pImage = &imageInfo;

                usageInfo.usage = type == ElementType::Image ? VK_IMAGE_USAGE_STORAGE_BIT : VK_IMAGE_USAGE_SAMPLED_BIT;
                viewInfo.image = view.image.image;
                viewInfo.viewType = view.image.viewType;
                viewInfo.format = view.image.format;
                viewInfo.components = type == ElementType::Image ? VkComponentMapping{} : VulkanEnumConvert::GetSwizzle(view.image.format);
                viewInfo.subresourceRange = view.image.range;
                imageInfo.layout = VK_IMAGE_LAYOUT_GENERAL;

                entry->resolution.x = math::max(1u, view.image.extent.width >> view.image.range.baseMipLevel);
                entry->resolution.y = math::max(1u, view.image.extent.height >> view.image.range.baseMipLevel);
                entry->resolution.z = math::max(1u, view.image.extent.depth >> view.image.range.baseMipLevel);
            }
            else
            {
                resourceInfo.data.pAddressRange = &view.memory;
                entry->resolution = PK_UINT3_ZERO;
            }

            VkHostAddressRangeEXT hostRange{ entry->data, PK_VK_MAX_RESOURCE_DESCRIPTOR_SIZE };
            VK_ASSERT_RESULT_CTX(vkWriteResourceDescriptorsEXT(m_device, 1u, &resourceInfo, &hostRange), "Failed to write resource descriptor!");
            
            auto* gpuAddress = m_resourceHeap.mappedPtr + index * m_properties.maxResourceStride;
            memcpy(gpuAddress, entry->data, m_properties.maxResourceStride);
        }

        m_resourceCache[mapIndex].value.pruneTick = m_currentPruneTick + m_pruneDelay;
        return m_resourceCache[mapIndex].value.index;
    }
}

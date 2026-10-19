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
            m_resourceHeap.arenaOffset = m_properties.resourceHeapOffset + PK_VK_MAX_DESCRIPTORS * m_properties.maxResourceStride;
            m_resourceHeap.arenaCapacity = math::align(resourceArenaSize, m_properties.maxResourceStride);

            VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferInfo.size = math::align(m_resourceHeap.arenaOffset + m_resourceHeap.arenaCapacity, m_properties.resourceHeapAlignment);
            bufferInfo.usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = queueFamilies.count;
            bufferInfo.pQueueFamilyIndices = queueFamilies.indices;
            PK_FATAL_ASSERT(bufferInfo.size <= m_properties.maxResourceHeapSize, "heap size '%ull' exceeds allowed max size 'ull'!", bufferInfo.size, m_properties.maxResourceHeapSize);
       
            VmaAllocationInfo allocationInfo{};
            VK_ASSERT_RESULT_CTX(vmaCreateBufferWithAlignment(m_allocator, &bufferInfo, &allocInfo, m_properties.resourceHeapAlignment, &m_resourceHeap.buffer, &m_resourceHeap.memory, &allocationInfo), "Failed to create a resource heap buffer!");
            VulkanSetObjectDebugName(m_device, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_resourceHeap.buffer, "PK_Resource_Heap");
            m_resourceHeap.mappedPtr = static_cast<uint8_t*>(allocationInfo.pMappedData);
            
            VkBufferDeviceAddressInfo addressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, m_resourceHeap.buffer };
            m_resourceHeap.bindInfo.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT;
            m_resourceHeap.bindInfo.heapRange = { vkGetBufferDeviceAddress(m_device, &addressInfo), bufferInfo.size };
            m_resourceHeap.bindInfo.reservedRangeOffset = 0ull;
            m_resourceHeap.bindInfo.reservedRangeSize = properties.descriptorHeap.minResourceHeapReservedRange;
        }

        // Create sampler heap
        {
            m_samplerHeap.arenaOffset = m_properties.samplerHeapOffset + PK_VK_MAX_SAMPLERS * m_properties.samplerStride;
            m_samplerHeap.arenaCapacity = PK_VK_MAX_SAMPLERS * 32u * m_properties.samplerStride;

            VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferInfo.size = math::align(m_samplerHeap.arenaOffset + m_samplerHeap.arenaCapacity, m_properties.samplerHeapAlignment);
            bufferInfo.usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferInfo.queueFamilyIndexCount = queueFamilies.count;
            bufferInfo.pQueueFamilyIndices = queueFamilies.indices;
            PK_FATAL_ASSERT(bufferInfo.size <= m_properties.maxSamplerHeapSize, "heap size '%ull' exceeds allowed max size 'ull'!", bufferInfo.size, m_properties.maxSamplerHeapSize);

            VmaAllocationInfo allocationInfo{};
            VK_ASSERT_RESULT_CTX(vmaCreateBufferWithAlignment(m_allocator, &bufferInfo, &allocInfo, m_properties.samplerHeapAlignment, &m_samplerHeap.buffer, &m_samplerHeap.memory, &allocationInfo), "Failed to create a sampler heap buffer!");
            VulkanSetObjectDebugName(m_device, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_samplerHeap.buffer, "PK_Sampler_Heap");
            m_samplerHeap.mappedPtr = static_cast<uint8_t*>(allocationInfo.pMappedData);
        
            VkBufferDeviceAddressInfo addressInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, nullptr, m_samplerHeap.buffer };
            m_samplerHeap.bindInfo.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT;
            m_samplerHeap.bindInfo.heapRange = { vkGetBufferDeviceAddress(m_device, &addressInfo), bufferInfo.size };
            m_samplerHeap.bindInfo.reservedRangeOffset = 0ull;
            m_samplerHeap.bindInfo.reservedRangeSize = properties.descriptorHeap.minSamplerHeapReservedRange;
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


    void VulkanDescriptorHeap::BeginRange()
    {
        m_rangeCounter++;
    }

    uint64_t VulkanDescriptorHeap::EndRange()
    {
        m_resourceHeap.ranges[m_rangeCounter % PK_VK_MAX_DESCRIPTOR_RANGES] = m_resourceHeap.arenaHead;
        m_samplerHeap.ranges[m_rangeCounter % PK_VK_MAX_DESCRIPTOR_RANGES] = m_samplerHeap.arenaHead;
        return m_rangeCounter;
    }

    void VulkanDescriptorHeap::FreeRange(uint64_t rangeIndex)
    {
        m_resourceHeap.arenaTail = math::max(m_resourceHeap.arenaTail, m_resourceHeap.ranges[rangeIndex % PK_VK_MAX_DESCRIPTOR_RANGES]);
        m_samplerHeap.arenaTail = math::max(m_samplerHeap.arenaTail, m_samplerHeap.ranges[rangeIndex % PK_VK_MAX_DESCRIPTOR_RANGES]);
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
            auto* gpuAddress = m_samplerHeap.mappedPtr + m_properties.samplerHeapOffset + index * m_properties.samplerStride;

            VkHostAddressRangeEXT hostRange{ cpuAddress, PK_VK_MAX_SAMPLER_DESCRIPTOR_SIZE };
            VK_ASSERT_RESULT_CTX(vkWriteSamplerDescriptorsEXT(m_device, 1u, &info, &hostRange), "Failed to write sampler descriptor!");

            memcpy(gpuAddress, cpuAddress, m_properties.samplerStride);
        }

        return index;
    }

    VulkanResourceView VulkanDescriptorHeap::GetSamplerView(const SamplerDescriptor& descriptor)
    {
        VulkanResourceView view{};
        view.image.samplerIndex = GetSamplerIndex(descriptor);
        view.UpdateHashImage();
        return view;
    }

    uint32_t VulkanDescriptorHeap::GetBindlessIndex(const VulkanResourceView& view, ShaderResourceType type)
    {
        if (type == ShaderResourceType::Sampler)
        {
            return view.image.samplerIndex;
        }

        return GetOrCacheResourceView(view, type);
    }

    uint3 VulkanDescriptorHeap::GetTextureSize(uint32_t bindlessIndex) const
    {
        return m_resourcePool[bindlessIndex]->resolution;
    }

    uint2 VulkanDescriptorHeap::AllocateDescriptorSet(const VulkanShader* shader, const VulkanDescriptorBinding* bindings, uint32_t count)
    {
        const auto resourceOffset = AllocateDescriptorRange(m_resourceHeap, shader->GetResourceSetSize(), m_properties.maxResourceStride);
        const auto samplerOffset = AllocateDescriptorRange(m_samplerHeap, shader->GetSamplerSetSize(), m_properties.samplerStride);
        const auto* offsets = shader->GetDescriptorOffsets();

        for (auto i = 0u; i < count; ++i)
        {
            const auto& binding = bindings[i];
            const auto& offset = offsets[i];

            for (auto j = 0u; j < binding.count; ++j)
            {
                const auto& view = binding.views[j];

                if (offset.resourceStride)
                {
                    const auto index = GetOrCacheResourceView(view, binding.type);
                    const auto* srcResource = m_resourcePool[index]->data;
                    memcpy(m_resourceHeap.mappedPtr + resourceOffset + offset.resourceOffset + j * offset.resourceStride, srcResource, offset.resourceStride);
                }

                if (offset.samplerStride)
                {
                    const auto* srcSampler = m_samplerCache[view.image.samplerIndex].value.data;
                    memcpy(m_samplerHeap.mappedPtr + samplerOffset + offset.samplerOffset + j * offset.samplerStride, srcSampler, offset.samplerStride);
                }
            }
        }

        return { resourceOffset, samplerOffset };
    }


    uint32_t VulkanDescriptorHeap::GetOrCacheResourceView(const VulkanResourceView& view, ShaderResourceType type)
    {
        // Pure resource descriptor: SamplerTexture in legacy shaders uses the exact same SAMPLED_IMAGE resource descriptor as Texture
        const auto cacheType = type == ShaderResourceType::SamplerTexture ? ShaderResourceType::Texture : type;
        const auto key = Hash::Mix(view.hash, (uint64_t)cacheType);
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

            resourceInfo.type = VulkanEnumConvert::GetDescriptorType(cacheType);

            if (cacheType == ShaderResourceType::Texture || cacheType == ShaderResourceType::Image)
            {
                viewInfo.pNext = &usageInfo;
                imageInfo.pView = &viewInfo;
                resourceInfo.data.pImage = &imageInfo;

                usageInfo.usage = cacheType == ShaderResourceType::Image ? VK_IMAGE_USAGE_STORAGE_BIT : VK_IMAGE_USAGE_SAMPLED_BIT;
                viewInfo.image = view.image.image;
                viewInfo.viewType = view.image.viewType;
                viewInfo.format = view.image.format;
                viewInfo.components = cacheType == ShaderResourceType::Image ? VkComponentMapping{} : VulkanEnumConvert::GetSwizzle(view.image.format);
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
            
            auto* gpuAddress = m_resourceHeap.mappedPtr + m_properties.resourceHeapOffset + index * m_properties.maxResourceStride;
            memcpy(gpuAddress, entry->data, m_properties.maxResourceStride);
        }

        m_resourceCache[mapIndex].value.pruneTick = m_currentPruneTick + m_pruneDelay;
        return m_resourceCache[mapIndex].value.index;
    }

    uint32_t VulkanDescriptorHeap::AllocateDescriptorRange(DescriptorHeap& heap, uint32_t size, uint32_t alignment)
    {
        if (size)
        {
            size = math::align(size, alignment);
            PK_DEBUG_FATAL_ASSERT(heap.arenaHead + size - heap.arenaTail <= heap.arenaCapacity, "Descriptor heap arena exhausted!");

            auto offset = heap.arenaHead % heap.arenaCapacity;

            if (offset + size > heap.arenaCapacity)
            {
                heap.arenaHead += heap.arenaCapacity - offset;
                offset = 0u;
            }

            heap.arenaHead += size;

            return heap.arenaOffset + offset;
        }

        return 0u;
    }
}

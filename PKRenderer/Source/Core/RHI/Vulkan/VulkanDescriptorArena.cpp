#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanDescriptorArena.h"

namespace PK
{
    VulkanDescriptorArena::VulkanDescriptorArena(const VulkanDriver* driver, const VulkanPhysicalDeviceProperties& properties, uint32_t size) :
        m_device(driver->device),
        m_allocator(driver->allocator),
        m_size(size)
    {
        if (m_size)
        {
            const auto& queueFamilies = driver->queues->GetSelectedFamilies();

            m_descriptorBufferOffsetAlignment = properties.descriptorBuffer.descriptorBufferOffsetAlignment;
            m_descriptorSizes[(uint32_t)ShaderResourceType::Invalid] = 0ull;
            m_descriptorSizes[(uint32_t)ShaderResourceType::Sampler] = properties.descriptorBuffer.samplerDescriptorSize;
            m_descriptorSizes[(uint32_t)ShaderResourceType::SamplerTexture] = properties.descriptorBuffer.combinedImageSamplerDescriptorSize;
            m_descriptorSizes[(uint32_t)ShaderResourceType::Texture] = properties.descriptorBuffer.sampledImageDescriptorSize;
            m_descriptorSizes[(uint32_t)ShaderResourceType::Image] = properties.descriptorBuffer.storageImageDescriptorSize;
            m_descriptorSizes[(uint32_t)ShaderResourceType::ConstantBuffer] = properties.descriptorBuffer.uniformBufferDescriptorSize;
            m_descriptorSizes[(uint32_t)ShaderResourceType::StorageBuffer] = properties.descriptorBuffer.storageBufferDescriptorSize;
            m_descriptorSizes[(uint32_t)ShaderResourceType::InputAttachment] = properties.descriptorBuffer.inputAttachmentDescriptorSize;
            m_descriptorSizes[(uint32_t)ShaderResourceType::AccelerationStructure] = properties.descriptorBuffer.accelerationStructureDescriptorSize;

            VkBufferCreateInfo bufferCreateInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
            bufferCreateInfo.size = m_size;
            bufferCreateInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bufferCreateInfo.queueFamilyIndexCount = queueFamilies.count;
            bufferCreateInfo.pQueueFamilyIndices = queueFamilies.indices;
            bufferCreateInfo.usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT |
                               VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT |
                               VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

            VmaAllocationCreateInfo allocationCreateInfo{};
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
            allocationCreateInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;

            VmaAllocationInfo allocationInfo{};
            VK_ASSERT_RESULT(vmaCreateBuffer(m_allocator, &bufferCreateInfo, &allocationCreateInfo, &m_buffer, &m_memory, &allocationInfo));
            m_mappedData = allocationInfo.pMappedData;

            VulkanSetObjectDebugName(driver->device, VK_OBJECT_TYPE_BUFFER, (uint64_t)m_buffer, "VulkanDescriptorArena");

            VkBufferDeviceAddressInfo addrInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
            addrInfo.buffer = m_buffer;
            m_deviceAddress = vkGetBufferDeviceAddress(m_device, &addrInfo);
        }
    }

    VulkanDescriptorArena::~VulkanDescriptorArena()
    {
        if (m_size)
        {
            vmaDestroyBuffer(m_allocator, m_buffer, m_memory);
        }
    }

    VkDeviceSize VulkanDescriptorArena::AllocateDescriptorSet(const VulkanDescriptorSetLayout* layout, const VulkanDescriptorBinding* bindings, const uint32_t bindingCount)
    {
        auto layoutSize = 0ull;
        vkGetDescriptorSetLayoutSizeEXT(m_device, layout->layout, &layoutSize);

        for (auto i = 0u; i < bindingCount; ++i)
        {
            if (bindings[i].isVariableSize && bindings[i].count > 0)
            {
                layoutSize += (bindings[i].count * m_descriptorSizes[(uint32_t)bindings[i].type]);
            }
        }

        PK_DEBUG_FATAL_ASSERT(m_inRange && m_size && layoutSize, "VulkanDescriptorArena: Failed to acquire a descriptor set!");

        const auto alignedSize = math::align(layoutSize, m_descriptorBufferOffsetAlignment);
        auto allocationOffset = m_bufferHead % m_size;

        if (allocationOffset + alignedSize > m_size)
        {
            m_bufferHead += (m_size - allocationOffset);
            allocationOffset = 0ull;
        }

        PK_DEBUG_FATAL_ASSERT(m_bufferHead - m_bufferFlushHead + alignedSize <= m_size, "VulkanDescriptorArena: overflow!");
        m_bufferHead += alignedSize;

        auto pSet = static_cast<uint8_t*>(m_mappedData) + allocationOffset;
        memset(pSet, 0, layoutSize);

        for (auto bindIndex = 0u; bindIndex < bindingCount; ++bindIndex)
        {
            const auto* bind = &bindings[bindIndex];
            const auto handles = bind->handles;
            const auto type = VulkanEnumConvert::GetDescriptorType(bind->type);
            const auto descriptorSize = m_descriptorSizes[(uint32_t)bind->type];
            auto bindingOffset = 0ull;
            vkGetDescriptorSetLayoutBindingOffsetEXT(m_device, layout->layout, bindIndex, &bindingOffset);

            for (auto arrayIndex = 0u; arrayIndex < bind->count; ++arrayIndex)
            {
                auto* handle = handles[arrayIndex];
                auto* pDescriptor = pSet + bindingOffset + (arrayIndex * descriptorSize);

                VkDescriptorDataEXT data{};
                VkDescriptorAddressInfoEXT bufferInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
                VkDescriptorImageInfo imageInfo{};
                
                switch (type)
                {
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    {
                        bufferInfo.address = handle->buffer.deviceAddress + handle->buffer.offset;
                        bufferInfo.range = handle->buffer.range;
                        data.pUniformBuffer = &bufferInfo;
                        break;
                    }
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    {
                        bufferInfo.address = handle->buffer.deviceAddress + handle->buffer.offset;
                        bufferInfo.range = handle->buffer.range;
                        data.pStorageBuffer = &bufferInfo;
                        break;
                    }
                    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    {
                        imageInfo.sampler = handle->image.sampler;
                        imageInfo.imageView = handle->image.view;
                        data.pCombinedImageSampler = &imageInfo;
                        break;
                    }
                    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    {
                        imageInfo.imageView = handle->image.view;
                        data.pSampledImage = &imageInfo;
                        break;
                    }
                    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    {
                        imageInfo.imageView = handle->image.view;
                        data.pStorageImage = &imageInfo;
                        break;
                    }
                    case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                    {
                        imageInfo.imageView = handle->image.view;
                        data.pInputAttachmentImage = &imageInfo;
                        break;
                    }
                    case VK_DESCRIPTOR_TYPE_SAMPLER:
                    {
                        data.pSampler = &handle->image.sampler;
                        break;
                    }
                    case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                    {
                        data.accelerationStructure = handle->acceleration.deviceAddress;
                        break;
                    }
                    default:
                    {
                        PK_FATAL_ERROR("Unsupported binding type!");
                    }
                }

                VkDescriptorGetInfoEXT descriptorInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
                descriptorInfo.type = type;
                descriptorInfo.data = data;
                vkGetDescriptorEXT(m_device, &descriptorInfo, descriptorSize, pDescriptor);
            }
        }
        
        return allocationOffset;
    }

    void VulkanDescriptorArena::BeginRange()
    {
        PK_DEBUG_FATAL_ASSERT(!m_inRange && m_rangeHead - m_rangeFlushHead < MAX_RANGES, "VulkanDescriptorArena: Failed to begin allocation range!");
        m_inRange = true;
        auto& range = m_ranges[m_rangeHead % MAX_RANGES];
        range.offset = m_bufferHead;
        range.size = 0ull;
    }

    uint64_t VulkanDescriptorArena::EndRange()
    {
        PK_DEBUG_FATAL_ASSERT(m_inRange, "VulkanDescriptorArena: Failed to end allocation range!");
        auto index = m_rangeHead;
        auto& range = m_ranges[m_rangeHead % MAX_RANGES];
        range.size = m_bufferHead - range.offset;
        m_inRange = false;
        ++m_rangeHead;
        return index;
    }

    void VulkanDescriptorArena::FreeRange(uint64_t rangeIndex)
    {
        if (rangeIndex >= m_rangeFlushHead)
        {
            m_rangeMask[rangeIndex % MAX_RANGES] = true;

            if (rangeIndex == m_rangeFlushHead)
            {
                while (m_rangeMask[m_rangeFlushHead % MAX_RANGES])
                {
                    m_rangeMask[m_rangeFlushHead % MAX_RANGES] = false;
                    m_bufferFlushHead += m_ranges[m_rangeFlushHead % MAX_RANGES].size;
                    ++m_rangeFlushHead;
                }
            }
        }
    }
}
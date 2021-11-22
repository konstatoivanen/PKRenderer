#include "PrecompiledHeader.h"
#include "VulkanDescriptorCache.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    VulkanDescriptorCache::VulkanDescriptorCache(VkDevice device, uint64_t pruneDelay, size_t maxSets, std::initializer_list<std::pair<const VkDescriptorType, size_t>> poolSizes) :
        m_device(device), 
        m_maxSets(maxSets), 
        m_poolSizes(poolSizes),
        m_pruneDelay(pruneDelay)
    {
        GrowPool({});
    }

    const VulkanDescriptorSet* VulkanDescriptorCache::GetDescriptorSet(const VulkanDescriptorSetLayout* layout, 
                                                                       const DescriptorSetKey& key,
                                                                       const VulkanExecutionGate& gate)
    {
        auto nextPruneTick = m_currentPruneTick + m_pruneDelay;
        auto iterator = m_sets.find(key);
        VulkanDescriptorSet* value = nullptr;

        if (iterator != m_sets.end() && iterator->second.set != VK_NULL_HANDLE)
        {
            iterator->second.pruneTick = nextPruneTick;
            value = &iterator->second;
        }
        else
        {
            VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
            allocInfo.pNext = nullptr;
            allocInfo.descriptorPool = m_currentPool->pool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts = &layout->layout;

            VkDescriptorSet vkdescriptorset;
            GetDescriptorSets(&allocInfo, &vkdescriptorset, gate, false);

            m_sets[key] = { vkdescriptorset, nextPruneTick };
            value = &m_sets.at(key);

            VkWriteDescriptorSet writes[PK_MAX_DESCRIPTORS_PER_SET]{};
            VkDescriptorBufferInfo buffers[PK_MAX_DESCRIPTORS_PER_SET];
            VkDescriptorImageInfo images[PK_MAX_DESCRIPTORS_PER_SET];
            auto count = 0u;

            for (; count < PK_MAX_DESCRIPTORS_PER_SET; ++count)
            {
                if (key.bindings[count].count == 0)
                {
                    break;
                }

                auto* bind = &key.bindings[count];
                auto* buffer = &buffers[count];
                auto* image = &images[count];
                auto* write = &writes[count];
                auto type = EnumConvert::GetDescriptorType(bind->type);
                
                write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write->dstArrayElement = 0;
                write->descriptorCount = bind->count;
                write->descriptorType = type;
                write->dstBinding = bind->binding;
                write->dstSet = value->set;

                image->sampler = bind->handle->sampler;
                image->imageView = bind->handle->imageView;
                image->imageLayout = bind->handle->imageLayout;
                buffer->buffer = bind->handle->buffer;
                buffer->range = bind->handle->bufferRange;
                buffer->offset = bind->handle->bufferOffset;

                switch (type)
                {
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                        write->pBufferInfo = buffer;
                        break;

                    case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    case VK_DESCRIPTOR_TYPE_SAMPLER:
                        write->pImageInfo = image;
                        break;

                    default: 
                        PK_THROW_ERROR("Binding type not yet implemented!");
                }
            }

            vkUpdateDescriptorSets(m_device, count, writes, 0, nullptr);
        }
        
        return value;
    }

    void VulkanDescriptorCache::Prune()
    {
        m_currentPruneTick++;

        decltype(m_extinctPools) extinctPools;
        extinctPools.swap(m_extinctPools);

        for (auto& pool : extinctPools)
        {
            if (!pool.executionGate.IsCompleted())
            {
                m_extinctPools.push_back(pool);
            }
        }

        for (auto& kv : m_sets)
        {
            auto& key = kv.first;
            auto& value = kv.second;

            if (value.set != VK_NULL_HANDLE && value.executionGate.IsCompleted() && value.pruneTick < m_currentPruneTick)
            {
                value.executionGate.Invalidate();
                vkFreeDescriptorSets(m_device, m_currentPool->pool, 1, &value.set);
            }
        }
    }

    void VulkanDescriptorCache::GrowPool(const VulkanExecutionGate& executionGate)
    {
        if (m_currentPool != nullptr)
        {
            m_extinctPools.push_back({ m_currentPool, executionGate });
            m_currentPool = nullptr;
        }

        m_sizeMultiplier++;

        std::vector<VkDescriptorPoolSize> pPoolSizes{};
        pPoolSizes.resize(m_poolSizes.size());
        auto i = 0u;

        for (auto& kv : m_poolSizes)
        {
            pPoolSizes[i].type = kv.first;
            pPoolSizes[i++].descriptorCount = (uint32_t)(kv.second * m_sizeMultiplier);
        }

        VkDescriptorPoolCreateInfo createInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
        createInfo.maxSets = (uint32_t)(m_maxSets * m_sizeMultiplier);
        createInfo.pPoolSizes = pPoolSizes.data();
        createInfo.poolSizeCount = (uint32_t)pPoolSizes.size();
        createInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        m_currentPool = CreateRef<VulkanDescriptorPool>(m_device, createInfo);
        m_sets.clear();
    }

    void VulkanDescriptorCache::GetDescriptorSets(const VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets, const VulkanExecutionGate& gate, bool throwOnFail)
    {
        auto result = vkAllocateDescriptorSets(m_device, pAllocateInfo, pDescriptorSets);

        switch (result)
        {
            case VK_SUCCESS:
                break;

            case VK_ERROR_FRAGMENTED_POOL:
            case VK_ERROR_OUT_OF_POOL_MEMORY:
                if (!throwOnFail)
                {
                    GrowPool(gate);
                    GetDescriptorSets(pAllocateInfo, pDescriptorSets, gate, true);
                    break;
                }
            default:
                PK_THROW_ERROR("Failed to allocate a descriptor set!");
        }
    }
}
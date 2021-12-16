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

    VulkanDescriptorCache::~VulkanDescriptorCache()
    {
        if (m_currentPool != nullptr)
        {
            delete m_currentPool;
        }

        for (auto& pool : m_extinctPools)
        {
            delete pool.pool;
        }
    }
    
    VulkanDescriptorCache::BindingArray::BindingArray(const VulkanBindHandle** bindings, size_t count, const VulkanExecutionGate& gate) : executionGate(gate)
    {
        resize(count);
        memcpy(data(), bindings, sizeof(bindings[0]) * count);
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
            return value;
        }

        VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.pNext = nullptr;
        allocInfo.descriptorPool = m_currentPool->pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout->layout;

        VkDescriptorSet vkdescriptorset;
        GetDescriptorSets(&allocInfo, &vkdescriptorset, gate, false);

        m_sets[key] = { vkdescriptorset, nextPruneTick, gate };
        value = &m_sets.at(key);

        VkWriteDescriptorSet writes[PK_MAX_DESCRIPTORS_PER_SET]{};
        auto count = 0u;

        auto imageCount = 0ull;
        auto bufferCount = 0ull;

        for (; count < PK_MAX_DESCRIPTORS_PER_SET; ++count)
        {
            if (key.bindings[count].count == 0)
            {
                break;
            }

            auto bind = key.bindings + count;
            auto handles = bind->count > 1 ? bind->handles : &bind->handle;
            auto* write = &writes[count];
            auto type = EnumConvert::GetDescriptorType(bind->type);
            
            write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write->dstArrayElement = 0;
            write->descriptorCount = bind->count;
            write->descriptorType = type;
            write->dstBinding = bind->binding;
            write->dstSet = value->set;

            switch (type)
            {
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                {
                    write->pBufferInfo = reinterpret_cast<decltype(write->pBufferInfo)>(bufferCount + 1);
                    auto newSize = bufferCount + bind->count;

                    if (m_writeBuffers.size() < newSize)
                    {
                        m_writeBuffers.resize(newSize);
                    }

                    auto buffers = m_writeBuffers.data() + bufferCount;
                    bufferCount = newSize;

                    for (auto i = 0; i < bind->count; ++i)
                    {
                        buffers[i].buffer = bind->handle->buffer;
                        buffers[i].offset = bind->handle->bufferOffset;
                        buffers[i].range = bind->handle->bufferRange;
                    }
                }
                break;

                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                {
                    write->pImageInfo = reinterpret_cast<decltype(write->pImageInfo)>(imageCount + 1);
                    auto newSize = imageCount + bind->count;

                    if (m_writeImages.size() < newSize)
                    {
                        m_writeImages.resize(newSize);
                    }

                    auto images = m_writeImages.data() + imageCount;
                    imageCount = newSize;

                    for (auto i = 0; i < bind->count; ++i)
                    {
                        images[i].sampler = handles[i]->sampler;
                        images[i].imageView = handles[i]->imageView;
                        images[i].imageLayout = handles[i]->imageLayout;
                    }
                }
                break;

                default: 
                    PK_THROW_ERROR("Binding type not yet implemented!");
            }
        }

        for (auto i = 0u; i < count; ++i)
        {
            auto w = writes + i;

            if (w->pBufferInfo != nullptr)
            {
                w->pBufferInfo = m_writeBuffers.data() + ((reinterpret_cast<size_t>(w->pBufferInfo) & 0xFFFF) - 1);
            }

            if (w->pImageInfo != nullptr)
            {
                w->pImageInfo = m_writeImages.data() + ((reinterpret_cast<size_t>(w->pImageInfo) & 0xFFFF)- 1);
            }
        }

        vkUpdateDescriptorSets(m_device, count, writes, 0, nullptr);
        return value;
    }

    const VulkanBindHandle** VulkanDescriptorCache::GetBindingArray(const VulkanBindHandle** bindings, size_t count, const VulkanExecutionGate& gate)
    {
        auto iter = m_bindingArrays.find({ bindings, count });

        if (iter != m_bindingArrays.end())
        {
            return iter->second.data();
        }
        
        auto&& value = BindingArray(bindings, count, gate);
        BindingArrayKey key = { value.data(), count };
        m_bindingArrays.insert_or_assign(key, value);
        
        return value.data();
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
                continue;
            }

            delete pool.pool;
        }

        for (auto& kv : m_sets)
        {
            auto& key = kv.first;
            auto& value = kv.second;

            if (value.set != VK_NULL_HANDLE && value.executionGate.IsCompleted() && value.pruneTick < m_currentPruneTick)
            {
                value.executionGate.Invalidate();
                VK_ASSERT_RESULT_CTX(vkFreeDescriptorSets(m_device, m_currentPool->pool, 1, &value.set), "Failed to free descriptor sets!");
                value.set = VK_NULL_HANDLE;
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
        m_currentPool = new VulkanDescriptorPool(m_device, createInfo);
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
#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "VulkanDescriptorCache.h"

namespace PK
{
    static uint32_t GetArraySize(const VulkanDescriptorCache::SetKey& key)
    {
        uint32_t variableSize = 0u;

        for (auto i = 0u; i < PK_RHI_MAX_DESCRIPTORS_PER_SET; ++i)
        {
            if (key.bindings[i].count == 0)
            {
                break;
            }

            if (key.bindings[i].isArray)
            {
                variableSize += key.bindings[i].count;
            }
        }

        return variableSize;
    }

    VulkanDescriptorCache::VulkanDescriptorCache(VkDevice device,
        uint64_t pruneDelay,
        size_t maxSets,
        std::initializer_list<std::pair<const VkDescriptorType, size_t>> poolSizes) :
        m_poolSizes(poolSizes),
        m_device(device),
        m_maxSets(maxSets),
        m_pruneDelay(pruneDelay),
        m_sets(1024)
    {
        GrowPool({});
    }

    const VulkanDescriptorSet* VulkanDescriptorCache::GetDescriptorSet(const VulkanDescriptorSetLayout* layout,
        const SetKey& key,
        const FenceRef& fence,
        const char* name)
    {
        auto nextPruneTick = m_currentPruneTick + m_pruneDelay;

        uint32_t index = 0u;

        if (!m_sets.AddKey(key, &index))
        {
            auto set = m_sets.GetValueAt(index);
            set->pruneTick = nextPruneTick;
            set->fence = fence;
            return set;
        }

        auto arraySize = GetArraySize(key);

        VkDescriptorSetVariableDescriptorCountAllocateInfo variableSizeInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
        variableSizeInfo.descriptorSetCount = 1u;
        variableSizeInfo.pDescriptorCounts = &arraySize;
        
        VkDescriptorSetAllocateInfo allocInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.pNext = arraySize > 0ull ? &variableSizeInfo : nullptr;
        allocInfo.descriptorPool = m_currentPool->pool;
        allocInfo.descriptorSetCount = 1u;
        allocInfo.pSetLayouts = &layout->layout;

        VkDescriptorSet vkdescriptorset;
        GetDescriptorSets(&allocInfo, &vkdescriptorset, fence, false);

        auto setName = layout->name + std::string(".") + std::string(name);
        VulkanSetObjectDebugName(m_device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)vkdescriptorset, setName.c_str());

        auto value = m_setsPool.New();
        value->set = vkdescriptorset;
        value->pruneTick = nextPruneTick;
        value->fence = fence;

        VkWriteDescriptorSet writes[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
        auto count = 0u;

        auto imageCount = 0ull;
        auto bufferCount = 0ull;
        auto accelerationStructureCount = 0ull;

        for (; count < PK_RHI_MAX_DESCRIPTORS_PER_SET; ++count)
        {
            if (key.bindings[count].count == 0)
            {
                break;
            }

            auto bind = key.bindings + count;
            auto handles = bind->isArray ? bind->handles : &bind->handle;
            auto* write = &writes[count];
            auto type = VulkanEnumConvert::GetDescriptorType(bind->type);

            write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write->dstArrayElement = 0;
            write->descriptorCount = bind->count;
            write->descriptorType = type;
            write->dstBinding = count;
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
                        buffers[i].buffer = bind->handle->buffer.buffer;
                        buffers[i].offset = bind->handle->buffer.offset;
                        buffers[i].range = bind->handle->buffer.range;
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
                    auto bindSampler = type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || type == VK_DESCRIPTOR_TYPE_SAMPLER;
                    auto bindImage = type != VK_DESCRIPTOR_TYPE_SAMPLER;

                    if (m_writeImages.size() < newSize)
                    {
                        m_writeImages.resize(newSize);
                    }

                    auto images = m_writeImages.data() + imageCount;
                    imageCount = newSize;

                    for (auto i = 0; i < bind->count; ++i)
                    {
                        images[i].sampler = bindSampler ? handles[i]->image.sampler : VK_NULL_HANDLE;
                        images[i].imageView = bindImage ? handles[i]->image.view : VK_NULL_HANDLE;
                        images[i].imageLayout = bindImage ? handles[i]->image.layout : VK_IMAGE_LAYOUT_UNDEFINED;
                    }
                }
                break;

                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                {
                    write->pNext = reinterpret_cast<decltype(write->pNext)>(accelerationStructureCount + 1);
                    auto newSize = accelerationStructureCount + bind->count;

                    if (m_writeAccerationStructures.size() < newSize)
                    {
                        m_writeAccerationStructures.resize(newSize);
                    }

                    auto accelerationStructures = m_writeAccerationStructures.data() + accelerationStructureCount;
                    accelerationStructureCount = newSize;

                    for (auto i = 0; i < bind->count; ++i)
                    {
                        accelerationStructures[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                        accelerationStructures[i].accelerationStructureCount = 1;
                        accelerationStructures[i].pAccelerationStructures = &handles[i]->acceleration.structure;
                    }

                    break;
                }

                default:
                    PK_THROW_ERROR("Unsuppored binding type!");
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
                w->pImageInfo = m_writeImages.data() + ((reinterpret_cast<size_t>(w->pImageInfo) & 0xFFFF) - 1);
            }

            if (w->pNext != nullptr)
            {
                w->pNext = m_writeAccerationStructures.data() + ((reinterpret_cast<size_t>(w->pNext) & 0xFFFF) - 1);
            }
        }

        vkUpdateDescriptorSets(m_device, count, writes, 0, nullptr);
        m_sets.SetValueAt(index, value);
        return value;
    }

    void VulkanDescriptorCache::Prune()
    {
        m_currentPruneTick++;

        for (auto i = (int32_t)m_extinctPools.size() - 1; i >= 0; --i)
        {
            if (!m_extinctPools.at(i).fence.IsComplete())
            {
                continue;
            }

            m_setsPool.Delete(m_extinctPools[i].indexMask);
            m_poolPool.Delete(m_extinctPools[i].poolIndex);
            auto n = (int32_t)m_extinctPools.size() - 1;

            if (i != n)
            {
                m_extinctPools[i] = m_extinctPools[n];
            }

            m_extinctPools.pop_back();
        }

        auto keyvalues = m_sets.GetKeyValues();

        for (int32_t i = (int32_t)keyvalues.count - 1; i >= 0; --i)
        {
            auto& value = keyvalues.values[i];

            if (!value->fence.IsComplete() || value->pruneTick >= m_currentPruneTick)
            {
                continue;
            }

            VK_ASSERT_RESULT_CTX(vkFreeDescriptorSets(m_device, m_currentPool->pool, 1, &value->set), "Failed to free descriptor sets!");
            value->set = VK_NULL_HANDLE;
            value->fence.Invalidate();
            m_setsPool.Delete(value);
            m_sets.RemoveAt((uint32_t)i);
        }
    }

    void VulkanDescriptorCache::GrowPool(const FenceRef& fence)
    {
        if (m_currentPool != nullptr)
        {
            m_extinctPools.push_back({ m_poolPool.GetIndex(m_currentPool), fence, m_setsPool.GetActiveMask() });
            m_currentPool = nullptr;
            m_sets.Clear();
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
        m_currentPool = m_poolPool.New(m_device, createInfo);
    }

    void VulkanDescriptorCache::GetDescriptorSets(VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets, const FenceRef& fence, bool throwOnFail)
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
                    GrowPool(fence);
                    pAllocateInfo->descriptorPool = m_currentPool->pool;
                    GetDescriptorSets(pAllocateInfo, pDescriptorSets, fence, true);
                    break;
                }
            default:
                PK_THROW_ERROR("Failed to allocate a descriptor set!");
        }
    }
}
#include "PrecompiledHeader.h"
#include "VulkanDescriptorCache.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"

namespace PK::Rendering::VulkanRHI::Systems
{
    using namespace Structs;

    static uint32_t GetArraySize(const DescriptorSetKey& key)
    {
        uint32_t variableSize = 0u;

        for (auto i = 0u; i < PK_MAX_DESCRIPTORS_PER_SET; ++i)
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

    VulkanDescriptorCache::VulkanDescriptorCache(VkDevice device, uint64_t pruneDelay, size_t maxSets, std::initializer_list<std::pair<const VkDescriptorType, size_t>> poolSizes) :
        m_device(device), 
        m_maxSets(maxSets), 
        m_poolSizes(poolSizes),
        m_pruneDelay(pruneDelay),
        m_sets(1024)
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

    const VulkanDescriptorSet* VulkanDescriptorCache::GetDescriptorSet(const VulkanDescriptorSetLayout* layout, 
                                                                       const DescriptorSetKey& key,
                                                                       const ExecutionGate& gate)
    {
        auto nextPruneTick = m_currentPruneTick + m_pruneDelay;
        VulkanDescriptorSet* value = nullptr;

        if (m_sets.TryGetValue(key, &value))
        {
            value->pruneTick = nextPruneTick;
            value->executionGate = gate;
            return value;
        }

        auto arraySize = GetArraySize(key);
        VkDescriptorSetVariableDescriptorCountAllocateInfo variableSizeInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
        VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.pNext = arraySize > 0ull ? &variableSizeInfo : nullptr;
        allocInfo.descriptorPool = m_currentPool->pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout->layout;
        variableSizeInfo.pDescriptorCounts = &arraySize;
        variableSizeInfo.descriptorSetCount = 1;
        VkDescriptorSet vkdescriptorset;
        GetDescriptorSets(&allocInfo, &vkdescriptorset, gate, false);

        value = m_setsPool.New();
        value->set = vkdescriptorset;
        value->pruneTick = nextPruneTick;
        value->executionGate = gate;
        m_sets.AddValue(key, value);

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
            auto handles = bind->isArray ? bind->handles : &bind->handle;
            auto* write = &writes[count];
            auto type = EnumConvert::GetDescriptorType(bind->type);
            
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

    void VulkanDescriptorCache::Prune()
    {
        m_currentPruneTick++;

        for (auto i = (int)m_extinctPools.size() - 1; i >= 0; --i)
        {
            if (m_extinctPools.at(i).executionGate.IsComplete())
            {
                auto n = m_extinctPools.size() - 1;

                for (auto& index : m_extinctPools[i].extinctSetIndices)
                {
                    m_setsPool.Delete(index);
                }

                delete m_extinctPools[i].pool;

                if (i != n)
                {
                    m_extinctPools[i] = m_extinctPools[n];
                }

                m_extinctPools.pop_back();
            }
        }

        auto keyvalues = m_sets.GetKeyValues();

        for (int32_t i = (int32_t)keyvalues.count - 1; i >= 0; --i)
        {
            auto& key = keyvalues.keys[i].key;
            auto& value = keyvalues.values[i];

            if (!value->executionGate.IsComplete() || value->pruneTick >= m_currentPruneTick)
            {
                continue;
            }

            VK_ASSERT_RESULT_CTX(vkFreeDescriptorSets(m_device, m_currentPool->pool, 1, &value->set), "Failed to free descriptor sets!");
            value->set = VK_NULL_HANDLE;
            value->executionGate.Invalidate();
            m_setsPool.Delete(value);
            m_sets.RemoveAt((uint32_t)i);
        }
    }

    void VulkanDescriptorCache::GrowPool(const ExecutionGate& executionGate)
    {
        if (m_currentPool != nullptr)
        {
            m_extinctPools.push_back({ m_currentPool, executionGate, m_setsPool.GetActiveIndices() });
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
        m_currentPool = new VulkanDescriptorPool(m_device, createInfo);
    }

    void VulkanDescriptorCache::GetDescriptorSets(VkDescriptorSetAllocateInfo* pAllocateInfo, VkDescriptorSet* pDescriptorSets, const ExecutionGate& gate, bool throwOnFail)
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
                    pAllocateInfo->descriptorPool = m_currentPool->pool;
                    GetDescriptorSets(pAllocateInfo, pDescriptorSets, gate, true);
                    break;
                }
            default:
                PK_THROW_ERROR("Failed to allocate a descriptor set!");
        }
    }
}
#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Utilities/FixedString.h"
#include "VulkanDescriptorCache.h"

namespace PK
{
    VulkanDescriptorCache::VulkanDescriptorCache(VkDevice device,
        uint64_t pruneDelay,
        size_t maxSets,
        std::initializer_list<std::pair<const VkDescriptorType, size_t>> poolSizes) :
        m_device(device),
        m_pruneDelay(pruneDelay)
    {
        m_poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        m_poolCreateInfo.pNext = nullptr;
        m_poolCreateInfo.maxSets = maxSets;
        m_poolCreateInfo.pPoolSizes = m_poolSizes;
        m_poolCreateInfo.poolSizeCount = poolSizes.size();
        m_poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        auto index = 0u;

        for (auto& size : poolSizes)
        {
            m_poolSizes[index].type = size.first;
            m_poolSizes[index++].descriptorCount = size.second;
        }

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
            auto set = m_sets[index].value;
            set->pruneTick = nextPruneTick;
            set->fence = fence;
            return set;
        }

        auto variableSize = 0u;

        for (auto i = 0u; i < PK_RHI_MAX_DESCRIPTORS_PER_SET; ++i)
        {
            if (key.bindings[i].isArray)
            {
                variableSize += key.bindings[i].count;
            }
        }

        VkDescriptorSetVariableDescriptorCountAllocateInfo variableSizeInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
        variableSizeInfo.descriptorSetCount = 1u;
        variableSizeInfo.pDescriptorCounts = &variableSize;
        
        VkDescriptorSetAllocateInfo allocInfo { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.pNext = variableSize > 0ull ? &variableSizeInfo : nullptr;
        allocInfo.descriptorPool = m_currentPool->pool;
        allocInfo.descriptorSetCount = 1u;
        allocInfo.pSetLayouts = &layout->layout;

        VkDescriptorSet vkdescriptorset;
        GetDescriptorSets(&allocInfo, &vkdescriptorset, fence, false);

        FixedString128 setName({ layout->name.c_str(), ".", name });
        VulkanSetObjectDebugName(m_device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)vkdescriptorset, setName.c_str());

        auto value = m_setsPool.New();
        value->set = vkdescriptorset;
        value->pruneTick = nextPruneTick;
        value->fence = fence;

        VkWriteDescriptorSet writes[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
        auto count = 0u;

        m_writeArena.Clear();

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
                    auto buffers = m_writeArena.Allocate<VkDescriptorBufferInfo>(bind->count);
                    write->pBufferInfo = buffers;

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
                    auto images = m_writeArena.Allocate<VkDescriptorImageInfo>(bind->count);
                    write->pImageInfo = images;

                    for (auto i = 0; i < bind->count; ++i)
                    {
                        images[i].sampler = type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || type == VK_DESCRIPTOR_TYPE_SAMPLER ? handles[i]->image.sampler : VK_NULL_HANDLE;
                        images[i].imageView = type != VK_DESCRIPTOR_TYPE_SAMPLER ? handles[i]->image.view : VK_NULL_HANDLE;
                        images[i].imageLayout = type != VK_DESCRIPTOR_TYPE_SAMPLER ? handles[i]->image.layout : VK_IMAGE_LAYOUT_UNDEFINED;
                    }
                }
                break;

                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                {
                    auto acclerationStructures = m_writeArena.Allocate<VkWriteDescriptorSetAccelerationStructureKHR>(bind->count);
                    write->pNext = acclerationStructures;

                    for (auto i = 0; i < bind->count; ++i)
                    {
                        acclerationStructures[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                        acclerationStructures[i].accelerationStructureCount = 1;
                        acclerationStructures[i].pAccelerationStructures = &handles[i]->acceleration.structure;
                    }

                    break;
                }

                default:
                    PK_THROW_ERROR("Unsuppored binding type!");
            }
        }

        vkUpdateDescriptorSets(m_device, count, writes, 0, nullptr);
        m_sets[index].value = value;
        return value;
    }

    void VulkanDescriptorCache::Prune()
    {
        m_currentPruneTick++;

        for (auto i = (int32_t)m_extinctPools.size() - 1; i >= 0; --i)
        {
            if (m_extinctPools.at(i).fence.IsComplete())
            {
                m_setsPool.Delete(m_extinctPools[i].indexMask);
                m_poolPool.Delete(m_extinctPools[i].poolIndex);
                auto n = (int32_t)m_extinctPools.size() - 1;

                if (i != n)
                {
                    m_extinctPools[i] = m_extinctPools[n];
                }

                m_extinctPools.pop_back();
            }
        }

        const auto count = (int32_t)m_sets.GetCount();

        for (int32_t i = count - 1; i >= 0; --i)
        {
            auto& value = m_sets[i].value;

            if (value->fence.IsComplete() && value->pruneTick < m_currentPruneTick)
            {
                VK_ASSERT_RESULT_CTX(vkFreeDescriptorSets(m_device, m_currentPool->pool, 1, &value->set), "Failed to free descriptor sets!");
                value->set = VK_NULL_HANDLE;
                value->fence.Invalidate();
                m_setsPool.Delete(value);
                m_sets.RemoveAt((uint32_t)i);
            }
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

        auto divisor = m_sizeMultiplier > 0 ? m_sizeMultiplier : 1u;
        m_sizeMultiplier++;

        for (auto i = 0u; i < VK_DESCRIPTOR_TYPE_COUNT; ++i)
        {
            m_poolSizes[i].descriptorCount = (m_poolSizes[i].descriptorCount / divisor) * m_sizeMultiplier;
        }

        m_poolCreateInfo.maxSets = (m_poolCreateInfo.maxSets / divisor) * m_sizeMultiplier;
        m_currentPool = m_poolPool.New(m_device, m_poolCreateInfo);
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

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

        m_currentPool = m_poolPool.New(m_device, m_poolCreateInfo);
        m_sizeMultiplier = 1u;
    }

    const VulkanDescriptorSet* VulkanDescriptorCache::GetDescriptorSet(const VulkanDescriptorSetLayout* layout,
        const DescriptorBinding* bindings,
        const uint32_t bindingCount,
        const FenceRef& fence,
        const char* name)
    {
        SetKey key;
        key.bindings = bindings;
        key.count = bindingCount;
        key.poolIndex = m_poolPool.GetIndex(m_currentPool);
        key.stageFlags = layout->stageFlags;

        uint32_t index = 0u;

        if (!m_sets.AddKey(key, &index))
        {
            auto set = m_sets[index].value;
            set->pruneTick = m_currentPruneTick + m_pruneDelay;
            set->fence = fence;
            return set;
        }

        auto variableSize = 0u;

        for (auto i = 0u; i < bindingCount; ++i)
        {
            if (bindings[i].isArray)
            {
                variableSize += bindings[i].count;
            }
        }

        FixedString128 setName({ layout->name.c_str(), ".", name });
        m_sets[index].value = AllocateDescriptorSet(layout->layout, variableSize, fence, setName.c_str());
        m_sets[index].key.poolIndex = m_poolPool.GetIndex(m_currentPool);
        m_sets[index].key.bindings = static_cast<DescriptorBinding*>(memcpy(m_bindingPool.NewArray(bindingCount), bindings, sizeof(DescriptorBinding) * bindingCount));

        m_writeArena.Clear();

        auto writes = m_writeArena.Allocate<VkWriteDescriptorSet>(bindingCount);

        for (auto i = 0u; i < bindingCount; ++i)
        {
            auto* bind = key.bindings + i;
            auto* write = writes + i;
            auto handles = bind->isArray ? bind->handles : &bind->handle;
            auto type = VulkanEnumConvert::GetDescriptorType(bind->type);

            write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write->dstArrayElement = 0;
            write->descriptorCount = bind->count;
            write->descriptorType = type;
            write->dstBinding = i;
            write->dstSet = m_sets[index].value->set;

            if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER || type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC || type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC)
            {
                auto buffers = m_writeArena.Allocate<VkDescriptorBufferInfo>(bind->count);
                write->pBufferInfo = buffers;

                for (auto i = 0; i < bind->count; ++i)
                {
                    buffers[i].buffer = bind->handle->buffer.buffer;
                    buffers[i].offset = bind->handle->buffer.offset;
                    buffers[i].range = bind->handle->buffer.range;
                }
                continue;
            }

            if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE || type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE || type == VK_DESCRIPTOR_TYPE_SAMPLER)
            {
                auto images = m_writeArena.Allocate<VkDescriptorImageInfo>(bind->count);
                write->pImageInfo = images;

                for (auto i = 0; i < bind->count; ++i)
                {
                    images[i].sampler = type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || type == VK_DESCRIPTOR_TYPE_SAMPLER ? handles[i]->image.sampler : VK_NULL_HANDLE;
                    images[i].imageView = type != VK_DESCRIPTOR_TYPE_SAMPLER ? handles[i]->image.view : VK_NULL_HANDLE;
                    images[i].imageLayout = type != VK_DESCRIPTOR_TYPE_SAMPLER ? handles[i]->image.layout : VK_IMAGE_LAYOUT_UNDEFINED;
                }
                continue;
            }

            if (type == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
            {
                auto acclerationStructures = m_writeArena.Allocate<VkWriteDescriptorSetAccelerationStructureKHR>(bind->count);
                write->pNext = acclerationStructures;

                for (auto i = 0; i < bind->count; ++i)
                {
                    acclerationStructures[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                    acclerationStructures[i].accelerationStructureCount = 1;
                    acclerationStructures[i].pAccelerationStructures = &handles[i]->acceleration.structure;
                }
                continue;
            }

            PK_THROW_ERROR("Unsuppored binding type!");
        }

        vkUpdateDescriptorSets(m_device, bindingCount, writes, 0, nullptr);
        return m_sets[index].value;
    }

    void VulkanDescriptorCache::Prune()
    {
        m_currentPruneTick++;
        const auto count = (int32_t)m_sets.GetCount();

        for (int32_t i = count - 1; i >= 0; --i)
        {
            auto& value = m_sets[i].value;

            if (value->fence.IsComplete() && value->pruneTick < m_currentPruneTick)
            {
                auto pool = m_poolPool[m_sets[i].key.poolIndex];
                VK_ASSERT_RESULT_CTX(vkFreeDescriptorSets(m_device, pool->pool, 1, &value->set), "Failed to free descriptor sets!");
                m_bindingPool.Delete(m_sets[i].key.bindings, m_sets[i].key.count);
                m_setsPool.Delete(value);
                m_sets.RemoveAt((uint32_t)i);
            }
        }

        for (auto i = 0u; i < m_poolPool.GetCapacity(); ++i)
        {
            if (m_poolPool.GetActiveMask().GetAt(i) && 
                m_poolPool[i]->fence.IsValid() && 
                m_poolPool[i]->fence.IsComplete() && 
                m_poolPool[i]->pruneTick < m_currentPruneTick)
            {
                m_poolPool.Delete(i);
            }
        }
    }

    VulkanDescriptorSet* VulkanDescriptorCache::AllocateDescriptorSet(VkDescriptorSetLayout layout, const uint32_t variableSize, const FenceRef& fence, const char* name)
    {
        VkDescriptorSetVariableDescriptorCountAllocateInfo variableSizeInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO };
        variableSizeInfo.descriptorSetCount = 1u;
        variableSizeInfo.pDescriptorCounts = &variableSize;

        VkDescriptorSetAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
        allocInfo.pNext = variableSize > 0ull ? &variableSizeInfo : nullptr;
        allocInfo.descriptorSetCount = 1u;
        allocInfo.pSetLayouts = &layout;

        VkDescriptorSet vkdescriptorset = VK_NULL_HANDLE;

        for (auto iteration = 0u; iteration <= 1u; ++iteration)
        {
            allocInfo.descriptorPool = m_currentPool->pool;
            auto result = vkAllocateDescriptorSets(m_device, &allocInfo, &vkdescriptorset);

            if (result != VK_SUCCESS)
            {
                vkdescriptorset = VK_NULL_HANDLE;
            }

            if (result == VK_ERROR_FRAGMENTED_POOL || result == VK_ERROR_OUT_OF_POOL_MEMORY)
            {
                auto divisor = m_sizeMultiplier++;

                for (auto i = 0u; i < PK_VK_MAX_DESCRIPTOR_TYPE_POOL_SIZES; ++i)
                {
                    m_poolSizes[i].descriptorCount = (m_poolSizes[i].descriptorCount / divisor) * m_sizeMultiplier;
                }

                m_poolCreateInfo.maxSets = (m_poolCreateInfo.maxSets / divisor) * m_sizeMultiplier;
                m_currentPool->fence = fence;
                m_currentPool->pruneTick = m_currentPruneTick + m_pruneDelay;
                m_currentPool = m_poolPool.New(m_device, m_poolCreateInfo);
                continue;
            }

            break;
        }

        if (vkdescriptorset == VK_NULL_HANDLE)
        {
            PK_THROW_ERROR("Failed to allocate a descriptor set!");
        }

        VulkanSetObjectDebugName(m_device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t)vkdescriptorset, name);

        auto value = m_setsPool.New();
        value->set = vkdescriptorset;
        value->pruneTick = m_currentPruneTick + m_pruneDelay;
        value->fence = fence;
        return value;
    }

    bool VulkanDescriptorCache::SetKey::operator==(const SetKey& other) const noexcept
    {
        return count == other.count &&
            poolIndex == other.poolIndex &&
            stageFlags == other.stageFlags &&
            memcmp(bindings, other.bindings, sizeof(DescriptorBinding) * count) == 0;
    }

    std::size_t VulkanDescriptorCache::SetKeyHash::operator()(const SetKey& k) const noexcept
    {
        constexpr uint64_t seed = 18446744073709551557ull;
        const auto hash0 = Hash::MurmurHash(&k.count, sizeof(k.count) + sizeof(k.poolIndex) + sizeof(k.stageFlags), seed);
        const auto hash1 = Hash::MurmurHash(k.bindings, sizeof(DescriptorBinding) * k.count, seed);
        const auto kMul = 0x9ddfea08eb382d69ULL;
        std::size_t a = (hash0 ^ hash1) * kMul;
        a ^= (a >> 47);
        std::size_t b = (hash1 ^ a) * kMul;
        b ^= (b >> 47);
        return b * kMul;
    }
}

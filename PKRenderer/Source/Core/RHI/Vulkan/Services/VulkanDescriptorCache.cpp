#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/Utilities/FixedString.h"
#include "VulkanDescriptorCache.h"

namespace PK
{
    static void GetArraySizes(const VulkanDescriptorCache::SetKey& key, 
        uint32_t* outVariableSize, 
        uint32_t* outBufferCount, 
        uint32_t* outImageCount, 
        uint32_t* outAccelerationStructureCount)
    {
        *outVariableSize = 0u;
        *outBufferCount = 0u;
        *outImageCount = 0u;
        *outAccelerationStructureCount = 0u;

        for (auto i = 0u; i < PK_RHI_MAX_DESCRIPTORS_PER_SET; ++i)
        {
            if (key.bindings[i].count == 0)
            {
                return;
            }

            if (key.bindings[i].isArray)
            {
                *outVariableSize += key.bindings[i].count;
            }

            switch (key.bindings[i].type)
            {
                case ShaderResourceType::ConstantBuffer:
                case ShaderResourceType::StorageBuffer:
                case ShaderResourceType::DynamicConstantBuffer:
                case ShaderResourceType::DynamicStorageBuffer:
                {
                    *outBufferCount += key.bindings[i].count;
                }
                break;
                case ShaderResourceType::Sampler:
                case ShaderResourceType::SamplerTexture:
                case ShaderResourceType::Texture:
                case ShaderResourceType::Image:
                {
                    *outImageCount += key.bindings[i].count;
                }
                break;
                case ShaderResourceType::AccelerationStructure:
                {
                    *outAccelerationStructureCount += key.bindings[i].count;
                }
                break;

                default: break;
            }
        }
    }

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

        uint32_t variableSize, bufferCount, imageCount, accelerationStructureCount;
        GetArraySizes(key, &variableSize, &bufferCount, &imageCount, &accelerationStructureCount);

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
        auto writeBuffers = m_writeArena.Allocate<VkDescriptorBufferInfo>(bufferCount);
        auto writeImages = m_writeArena.Allocate<VkDescriptorImageInfo>(imageCount);
        auto writeAccerationStructures = m_writeArena.Allocate<VkWriteDescriptorSetAccelerationStructureKHR>(accelerationStructureCount);

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
                    write->pBufferInfo = writeBuffers;

                    for (auto i = 0; i < bind->count; ++i)
                    {
                        writeBuffers->buffer = bind->handle->buffer.buffer;
                        writeBuffers->offset = bind->handle->buffer.offset;
                        writeBuffers->range = bind->handle->buffer.range;
                        writeBuffers++;
                    }
                }
                break;

                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                {
                    write->pImageInfo = writeImages;
                    auto bindSampler = type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER || type == VK_DESCRIPTOR_TYPE_SAMPLER;
                    auto bindImage = type != VK_DESCRIPTOR_TYPE_SAMPLER;

                    for (auto i = 0; i < bind->count; ++i)
                    {
                        writeImages->sampler = bindSampler ? handles[i]->image.sampler : VK_NULL_HANDLE;
                        writeImages->imageView = bindImage ? handles[i]->image.view : VK_NULL_HANDLE;
                        writeImages->imageLayout = bindImage ? handles[i]->image.layout : VK_IMAGE_LAYOUT_UNDEFINED;
                        writeImages++;
                    }
                }
                break;

                case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                {
                    write->pNext = writeAccerationStructures;

                    for (auto i = 0; i < bind->count; ++i)
                    {
                        writeAccerationStructures->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
                        writeAccerationStructures->accelerationStructureCount = 1;
                        writeAccerationStructures->pAccelerationStructures = &handles[i]->acceleration.structure;
                        writeAccerationStructures++;
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

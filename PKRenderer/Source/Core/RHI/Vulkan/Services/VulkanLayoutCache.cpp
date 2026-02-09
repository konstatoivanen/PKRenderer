#include "PrecompiledHeader.h"
#include "Core/Utilities/FixedString.h"
#include "Core/CLI/Log.h"
#include "VulkanLayoutCache.h"

namespace PK
{
    const VulkanDescriptorSetLayout* VulkanLayoutCache::GetSetLayout(const DescriptorSetLayoutKey& key)
    {
        auto index = 0u;
        if (!m_setLayoutMap.AddKey(key, &index))
        {
            auto layout = m_setLayoutPool[m_setLayoutMap[index].value];
            layout->referenceCount++;
            return layout;
        }

        VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
        VkDescriptorSetLayoutCreateInfo layoutCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
        VkDescriptorSetLayoutBinding bindings[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};
        VkDescriptorBindingFlags bindingFlags[PK_RHI_MAX_DESCRIPTORS_PER_SET]{};

        layoutCreateInfo.pNext = &bindingFlagsInfo;
        layoutCreateInfo.pBindings = bindings;
        bindingFlagsInfo.pBindingFlags = bindingFlags;

        uint32_t count = 0u;

        for (; count < PK_RHI_MAX_DESCRIPTORS_PER_SET; ++count)
        {
            if (key.counts[count] == 0)
            {
                break;
            }

            bindingFlags[count] = key.counts[count] >= PK_RHI_MAX_UNBOUNDED_SIZE ? VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT : 0u;
            bindings[count].binding = count;
            bindings[count].descriptorCount = key.counts[count];
            bindings[count].descriptorType = VulkanEnumConvert::GetDescriptorType(key.types[count]);
            bindings[count].stageFlags = key.stageFlags;
        }

        bindingFlagsInfo.bindingCount = layoutCreateInfo.bindingCount = count;

        FixedString64 layoutName("SetLayout%02d", index);
        auto newLayout = m_setLayoutPool.New(m_device, layoutCreateInfo, (VkShaderStageFlagBits)key.stageFlags, layoutName.c_str());
        newLayout->referenceCount = 1u;
        newLayout->releaseFence.Invalidate();
        m_setLayoutMap[index].value = m_setLayoutPool.GetIndex(newLayout);
        return newLayout;
    }

    const VulkanPipelineLayout* VulkanLayoutCache::GetPipelineLayout(const PipelineLayoutKey& key, const char* name)
    {
        auto index = 0u;
        if (!m_pipelineLayoutMap.AddKey(key, &index))
        {
            auto layout = m_pipelineLayoutPool[m_pipelineLayoutMap[index].value];
            layout->referenceCount++;
            return layout;
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutInfo.pSetLayouts = &key.setlayout;
        pipelineLayoutInfo.pPushConstantRanges = &key.pushConstantRange;
        pipelineLayoutInfo.setLayoutCount = 1u;
        pipelineLayoutInfo.pushConstantRangeCount = key.pushConstantRange.size > 0u ? 1u : 0u;

        auto newLayout = m_pipelineLayoutPool.New(m_device, pipelineLayoutInfo, FixedString64("%s.PipelineLayout", name).c_str());
        newLayout->referenceCount = 1u;
        newLayout->releaseFence.Invalidate();
        m_pipelineLayoutMap[index].value = m_pipelineLayoutPool.GetIndex(newLayout);
        return newLayout;
    }

    void VulkanLayoutCache::ReleaseSetLayout(const VulkanDescriptorSetLayout* layout, const FenceRef& releaseFence)
    {
        auto localLayout = m_setLayoutPool[m_setLayoutPool.GetIndex(layout)];
        localLayout->referenceCount--;

        if (localLayout->referenceCount <= 0u)
        {
            localLayout->releaseFence = releaseFence;
        }
    }

    void VulkanLayoutCache::ReleasePipelineLayout(const VulkanPipelineLayout* layout, const FenceRef& releaseFence)
    {
        auto localLayout = m_pipelineLayoutPool[m_pipelineLayoutPool.GetIndex(layout)];
        localLayout->referenceCount--;

        if (localLayout->referenceCount <= 0u)
        {
            localLayout->releaseFence = releaseFence;
        }
    }

    void VulkanLayoutCache::Prune()
    {
        for (int32_t i = m_pipelineLayoutMap.GetCount() - 1; i >= 0; --i)
        {
            auto layout = m_pipelineLayoutPool[m_pipelineLayoutMap[i].value];

            if (layout->referenceCount <= 0u && layout->releaseFence.WaitInvalidate(0u))
            {
                m_pipelineLayoutMap.RemoveAt(i);
                m_pipelineLayoutPool.Delete(layout);
            }
        }

        for (int32_t i = m_setLayoutMap.GetCount() - 1; i >= 0; --i)
        {
            auto layout = m_setLayoutPool[m_setLayoutMap[i].value];

            if (layout->referenceCount <= 0u && layout->releaseFence.WaitInvalidate(0u))
            {
                m_setLayoutMap.RemoveAt(i);
                m_setLayoutPool.Delete(layout);
            }
        }
    }
}

#include "PrecompiledHeader.h"
#include "Core/Utilities/FixedString.h"
#include "VulkanLayoutCache.h"

namespace PK
{
    const VulkanDescriptorSetLayout* VulkanLayoutCache::GetSetLayout(const DescriptorSetLayoutKey& key)
    {
        auto index = 0u;
        if (!m_setlayouts.AddKey(key, &index))
        {
            return m_setlayouts[index].value;
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
            bindings[count].descriptorType = key.types[count];
            bindings[count].stageFlags = key.stageFlags;
        }

        bindingFlagsInfo.bindingCount = layoutCreateInfo.bindingCount = count;
        FixedString64 layoutName("SetLayout%02d", index);
        auto value = m_setLayoutPool.New(m_device, layoutCreateInfo, (VkShaderStageFlagBits)key.stageFlags, layoutName.c_str());
        m_setlayouts[index].value = value;
        return value;
    }

    const VulkanPipelineLayout* VulkanLayoutCache::GetPipelineLayout(const PipelineLayoutKey& key)
    {
        auto index = 0u;
        if (!m_pipelineLayouts.AddKey(key, &index))
        {
            return m_pipelineLayouts[index].value;
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutInfo.pSetLayouts = key.setlayouts;
        pipelineLayoutInfo.pPushConstantRanges = key.pushConstants;
        pipelineLayoutInfo.setLayoutCount = PK_RHI_MAX_DESCRIPTOR_SETS;
        pipelineLayoutInfo.pushConstantRangeCount = PK_RHI_MAX_PUSH_CONSTANTS;

        for (auto i = 0u; i < PK_RHI_MAX_DESCRIPTOR_SETS; ++i)
        {
            if (key.setlayouts[i] == nullptr)
            {
                pipelineLayoutInfo.setLayoutCount = i;
                break;
            }
        }

        for (auto i = 0u; i < PK_RHI_MAX_PUSH_CONSTANTS; ++i)
        {
            if (key.pushConstants[i].size == 0)
            {
                pipelineLayoutInfo.pushConstantRangeCount = i;
                break;
            }
        }

        auto value = m_pipelineLayoutPool.New(m_device, pipelineLayoutInfo);
        m_pipelineLayouts[index].value = value;
        return value;
    }
}
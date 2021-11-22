#include "PrecompiledHeader.h"
#include "VulkanShader.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "SPIRV-Reflect/spirv_reflect.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Rendering::VulkanRHI::Utilities;
    using namespace PK::Rendering::Structs;

    struct ReflectionData
    {
        SpvReflectShaderModule m_modules[(int)ShaderStage::MaxCount]{};
        std::map<uint32_t, SpvReflectDescriptorBinding*> uniqueBindings;
        std::map<uint32_t, uint32_t> setStageFlags;
        uint32_t setCount = 0u;
    };

    static void GetReflectionModule(ReflectionData& reflection, ShaderStage stage, const std::vector<uint32_t>& spriv)
    {
        auto* module = &reflection.m_modules[(uint32_t)stage];
        PK_THROW_ASSERT(spvReflectCreateShaderModule(spriv.size() * sizeof(uint32_t), spriv.data(), module) == SPV_REFLECT_RESULT_SUCCESS, "Shader spirv reflection failed!");
    }

    static void GetUniqueBindings(ReflectionData& reflection, ShaderStage stage)
    {
        auto* module = &reflection.m_modules[(uint32_t)stage];
        auto stageFlags = EnumConvert::GetShaderStage(stage);

        uint32_t bindingCount = 0u;
        uint32_t setCount = 0u;

        spvReflectEnumerateEntryPointDescriptorBindings(module, module->entry_point_name, &bindingCount, nullptr);
        spvReflectEnumerateEntryPointDescriptorSets(module, module->entry_point_name, &setCount, nullptr);

        if (setCount > reflection.setCount)
        {
            reflection.setCount = setCount;
        }

        std::vector<SpvReflectDescriptorBinding*> activeBindings{};
        activeBindings.resize(setCount);
        spvReflectEnumerateEntryPointDescriptorBindings(module, module->entry_point_name, &bindingCount, activeBindings.data());

        for (auto i = 0u; i < bindingCount; ++i)
        {
            auto* binding = activeBindings.at(i);

            auto nameId = StringHashID::StringToID(binding->name);
            reflection.setStageFlags[binding->set] |= stageFlags;

            if (reflection.uniqueBindings.count(nameId) <= 0)
            {
                reflection.uniqueBindings[nameId] = binding;
            }
        }
    }

    static void GetVertexAttributes(ReflectionData& reflection, ShaderStage stage, VertexLayout* layout)
    {
        if (stage != ShaderStage::Vertex)
        {
            return;
        }

        auto* module = &reflection.m_modules[(uint32_t)stage];

        auto count = 0u;
        spvReflectEnumerateEntryPointInputVariables(module, module->entry_point_name, &count, nullptr);

        std::vector<SpvReflectInterfaceVariable*> variables;
        variables.resize(count);

        spvReflectEnumerateEntryPointInputVariables(module, module->entry_point_name, &count, variables.data());

        std::vector<VertexElement> elements;

        for (auto* variable : variables)
        {
            elements.emplace_back(EnumConvert::GetElementType((VkFormat)variable->format), std::string(variable->name), variable->location);
        }

        *layout = VertexLayout(elements);
    }

    static void BuildPipelineLayout(VkDevice device, 
                                    ReflectionData& reflection, 
                                    Ref<VulkanDescriptorSetLayout>* outsets, 
                                    ResourceLayout* outResourceLayouts,
                                    Ref<VulkanPipelineLayout>& m_pipelineLayout)
    {
        if (reflection.setCount <= 0)
        {
            return;
        }

        std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> bindingMap;
        std::map<uint32_t, std::vector<ResourceElement>> resourceLayouts;

        for (auto& kv : reflection.uniqueBindings)
        {
            VkDescriptorSetLayoutBinding descriptorBinding{};
            descriptorBinding.binding = kv.second->binding;
            descriptorBinding.descriptorCount = kv.second->count;
            descriptorBinding.descriptorType = (VkDescriptorType)kv.second->descriptor_type;
            descriptorBinding.stageFlags = reflection.setStageFlags[kv.second->set];

            if (kv.second->set >= PK_MAX_DESCRIPTOR_SETS)
            {
                PK_LOG_WARNING("Warning has a descriptor set outside of supported range (%i / %i)", kv.second->set, PK_MAX_DESCRIPTOR_SETS);
                continue;
            }

            resourceLayouts[kv.second->set].emplace_back(EnumConvert::GetResourceType(descriptorBinding.descriptorType, 1), 
                                                         std::string(kv.second->name), 
                                                         kv.second->binding, 
                                                         kv.second->count);

            bindingMap[kv.second->set].push_back(descriptorBinding);
        }

        std::vector<VkDescriptorSetLayout> layouts{};
        layouts.reserve(bindingMap.size());

        for (auto& kv : bindingMap)
        {
            VkDescriptorSetLayoutCreateInfo descriptorsetLayoutCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
            descriptorsetLayoutCreateInfo.bindingCount = (uint32_t)kv.second.size();
            descriptorsetLayoutCreateInfo.pBindings = kv.second.data();
            auto descriptorSet = CreateRef<VulkanDescriptorSetLayout>(device, descriptorsetLayoutCreateInfo);

            layouts.push_back(descriptorSet->layout);

            outResourceLayouts[kv.first] = ResourceLayout(resourceLayouts[kv.first]);
            outsets[kv.first] = descriptorSet;
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutInfo.setLayoutCount = (uint32_t)layouts.size();
        pipelineLayoutInfo.pSetLayouts = layouts.data();
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        m_pipelineLayout = CreateRef<VulkanPipelineLayout>(device, pipelineLayoutInfo);
    }

    VulkanShader::VulkanShader(VkDevice device, const VulkanShaderCreateInfo& createInfo)
    {
        ReflectionData reflectionData{};

        for (auto i = 0u; i < (int)ShaderStage::MaxCount; ++i)
        {
            if (createInfo.spirv[i].size() <= 0)
            {
                m_modules[i] = nullptr;
                continue;
            }

            m_modules[i] = CreateRef<VulkanShaderModule>(device, EnumConvert::GetShaderStage((ShaderStage)i), createInfo.spirv[i]);
            GetReflectionModule(reflectionData, (ShaderStage)i, createInfo.spirv[i]);
            GetUniqueBindings(reflectionData, (ShaderStage)i);
            GetVertexAttributes(reflectionData, (ShaderStage)i, &m_vertexLayout);
        }

        BuildPipelineLayout(device, reflectionData, m_descriptorSetLayouts, m_resourceLayouts, m_pipelineLayout);

        m_type = m_modules[(int)ShaderStage::Compute] != nullptr ? ShaderType::Compute : ShaderType::Graphics;
    }
}
#include "PrecompiledHeader.h"
#include "VulkanShader.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/GraphicsAPI.h"
#include <PKAssets/PKAssetLoader.h>

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Rendering::VulkanRHI::Utilities;
    using namespace PK::Rendering::Structs;

    VulkanShader::VulkanShader(void* base, PK::Assets::Shader::PKShaderVariant* variant) : m_device(GraphicsAPI::GetActiveDriver<VulkanDriver>()->device)
    {
        m_stageFlags = 0u;

        for (auto i = 0u; i < (int)ShaderStage::MaxCount; ++i)
        {
            if (variant->sprivSizes[i] == 0)
            {
                m_modules[i] = nullptr;
                continue;
            }

            auto spirvSize = variant->sprivSizes[i];
            auto* spirv = reinterpret_cast<uint32_t*>(variant->sprivBuffers[i].Get(base));
            m_modules[i] = new VulkanShaderModule(m_device, EnumConvert::GetShaderStage((ShaderStage)i), spirv, spirvSize);
            m_stageFlags |= 1 << i;
        }

        std::vector<BufferElement> vertexElements;

        for (auto i = 0; i < PK::Assets::PK_ASSET_MAX_VERTEX_ATTRIBUTES; ++i)
        {
            auto attribute = &variant->vertexAttributes[i];

            if (attribute->type != ElementType::Invalid)
            {
                vertexElements.emplace_back(attribute->type, attribute->name, 1, (byte)attribute->location);
            }
        }

        m_vertexLayout = BufferLayout(vertexElements);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        VkDescriptorSetLayout layouts[PK_MAX_DESCRIPTOR_SETS];
        std::vector<VkPushConstantRange> pushConstantRanges;
        m_descriptorSetCount = variant->descriptorSetCount;

        if (variant->descriptorSetCount > 0)
        {
            VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO };
            VkDescriptorSetLayoutCreateInfo layoutCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
            VkDescriptorSetLayoutBinding bindings[PK_MAX_DESCRIPTORS_PER_SET]{};
            VkDescriptorBindingFlags bindingFlags[PK_MAX_DESCRIPTORS_PER_SET]{};
            std::vector<ResourceElement> elements;

            layoutCreateInfo.pNext = &bindingFlagsInfo;
            layoutCreateInfo.pBindings = bindings;
            bindingFlagsInfo.pBindingFlags = bindingFlags;

            auto* pDescriptorSets = variant->descriptorSets.Get(base);

            for (auto i = 0u; i < variant->descriptorSetCount; ++i)
            {
                auto pDescriptorSet = pDescriptorSets + i;
                auto pDescriptors = pDescriptorSet->descriptors.Get(base);
                elements.clear();

                for (auto j = 0u; j < pDescriptorSet->descriptorCount; ++j)
                {
                    bindingFlags[j] = pDescriptors[j].count >= PK_MAX_UNBOUNDED_SIZE ? VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT : 0u;
                    bindings[j].binding = j;
                    bindings[j].descriptorCount = pDescriptors[j].count;
                    bindings[j].descriptorType = EnumConvert::GetDescriptorType(pDescriptors[j].type);
                    bindings[j].stageFlags = EnumConvert::GetShaderStageFlags(pDescriptorSet->stageflags);
                    elements.emplace_back(pDescriptors[j].type, std::string(pDescriptors[j].name), pDescriptors[j].count);
                }

                bindingFlagsInfo.bindingCount = layoutCreateInfo.bindingCount = pDescriptorSet->descriptorCount;
                m_descriptorSetLayouts[i] = CreateScope<VulkanDescriptorSetLayout>(m_device, layoutCreateInfo, EnumConvert::GetShaderStageFlags(pDescriptorSet->stageflags));
                layouts[i] = m_descriptorSetLayouts[i]->layout;
                m_resourceLayouts[i] = ResourceLayout(elements);
            }

            pipelineLayoutInfo.setLayoutCount = variant->descriptorSetCount;
            pipelineLayoutInfo.pSetLayouts = layouts;
        }

        if (variant->constantVariableCount > 0)
        {
            std::vector<ConstantVariable> variables;

            auto pVariables = variant->constantVariables.Get(base);

            for (auto i = 0u; i < variant->constantVariableCount; ++i)
            {
                auto pVariable = pVariables + i;
                variables.emplace_back(pVariable->name, pVariable->size, pVariable->offset, pVariable->stageFlags);
            
                VkPushConstantRange range{};
                range.stageFlags = EnumConvert::GetShaderStageFlags(pVariable->stageFlags);
                range.offset = pVariable->offset;
                range.size = pVariable->size;
                pushConstantRanges.push_back(range);
            }

            m_constantLayout = ConstantBufferLayout(variables);
            pipelineLayoutInfo.pushConstantRangeCount = (uint32_t)pushConstantRanges.size();
            pipelineLayoutInfo.pPushConstantRanges = pushConstantRanges.data();
        }

        m_pipelineLayout = CreateScope<VulkanPipelineLayout>(m_device, pipelineLayoutInfo);

        m_type = m_modules[(int)ShaderStage::Compute] != nullptr ? ShaderType::Compute : ShaderType::Graphics;
    }

    VulkanShader::~VulkanShader()
    {
        Dispose();
    }

    void VulkanShader::Dispose()
    {
        auto driver = GraphicsAPI::GetActiveDriver<VulkanDriver>();

        for (auto& module : m_modules)
        {
            if (module != nullptr)
            {
                driver->disposer->Dispose(module, driver->commandBufferPool->GetCurrent()->GetOnCompleteGate());
            }
        }
    }
}
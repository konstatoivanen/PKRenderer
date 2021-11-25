#include "PrecompiledHeader.h"
#include "VulkanShader.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include <PKAssetLoader.h>

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Rendering::VulkanRHI::Utilities;
    using namespace PK::Rendering::Structs;

    VulkanShader::VulkanShader(VkDevice device, const char* path) : m_device(device)
    {
        Import(path);
    }

    void VulkanShader::Import(const char* path)
    {
        auto asset = PK::Assets::OpenAsset(path);

        PK_THROW_ASSERT(asset.header->type == PK::Assets::PKAssetType::Shader, "Trying to read a shader from a non shader file!")

        auto shader = PK::Assets::ReadAsShader(&asset);
        auto base = asset.rawData;

        PK_THROW_ASSERT(shader->variantcount == 0, "Trying to read a shader with 0 variants!");

        auto variant = shader->variants.Get(base);

        for (auto i = 0u; i < (int)ShaderStage::MaxCount; ++i)
        {
            if (variant->sprivSizes[i] == 0)
            {
                m_modules[i] = nullptr;
                continue;
            }

            auto spirvSize = variant->sprivSizes[i];
            auto* spirv = reinterpret_cast<uint32_t*>(variant->sprivBuffers[i].Get(base));
            m_modules[i] = CreateRef<VulkanShaderModule>(m_device, EnumConvert::GetShaderStage((ShaderStage)i), spirv, spirvSize);
        }

        std::vector<VertexElement> vertexElements;

        for (auto i = 0; i < PK::Assets::PK_ASSET_MAX_VERTEX_ATTRIBUTES; ++i)
        {
            auto attribute = &variant->vertexAttributes[i];
            vertexElements.emplace_back(attribute->type, attribute->name, attribute->location);
        }

        m_vertexLayout = VertexLayout(vertexElements);

        if (variant->descriptorSetCount > 0)
        {
            std::vector<VkDescriptorSetLayout> layouts{};
            layouts.reserve(variant->descriptorSetCount);

            auto* pDescriptorSets = variant->descriptorSets.Get(base);
        
            for (auto i = 0u; i < variant->descriptorSetCount; ++i)
            {
                auto pDescriptorSet = pDescriptorSets + i;
                auto pDescriptors = pDescriptorSet->descriptors.Get(base);
                std::vector<VkDescriptorSetLayoutBinding> bindings;
                std::vector<ResourceElement> elements;
                
                for (auto j = 0u; j < pDescriptorSet->descriptorCount; ++i)
                {
                    VkDescriptorSetLayoutBinding descriptorBinding{};
                    descriptorBinding.binding = pDescriptors[j].binding;
                    descriptorBinding.descriptorCount = pDescriptors[j].count;
                    descriptorBinding.descriptorType = EnumConvert::GetDescriptorType(pDescriptors[j].type);
                    descriptorBinding.stageFlags = EnumConvert::GetShaderStageFlags(pDescriptorSet->stageflags);
                    bindings.push_back(descriptorBinding);
                    elements.emplace_back(pDescriptors[i].type, std::string(pDescriptors[i].name), pDescriptors[i].binding, pDescriptors[i].count);
                }

                VkDescriptorSetLayoutCreateInfo descriptorsetLayoutCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
                descriptorsetLayoutCreateInfo.bindingCount = (uint32_t)bindings.size();
                descriptorsetLayoutCreateInfo.pBindings = bindings.data();
                auto descriptorSet = CreateRef<VulkanDescriptorSetLayout>(m_device, descriptorsetLayoutCreateInfo);

                layouts.push_back(descriptorSet->layout);
                m_descriptorSetLayouts[pDescriptorSet->set] = descriptorSet;
                m_resourceLayouts[pDescriptorSet->set] = ResourceLayout(elements);
            }

            VkPipelineLayoutCreateInfo pipelineLayoutInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
            pipelineLayoutInfo.setLayoutCount = (uint32_t)layouts.size();
            pipelineLayoutInfo.pSetLayouts = layouts.data();
            pipelineLayoutInfo.pushConstantRangeCount = 0;
            pipelineLayoutInfo.pPushConstantRanges = nullptr;
            m_pipelineLayout = CreateRef<VulkanPipelineLayout>(m_device, pipelineLayoutInfo);
        }

        m_type = m_modules[(int)ShaderStage::Compute] != nullptr ? ShaderType::Compute : ShaderType::Graphics;

        PK::Assets::CloseAsset(&asset);
    }
}
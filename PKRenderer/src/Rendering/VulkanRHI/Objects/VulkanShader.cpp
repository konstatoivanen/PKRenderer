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

        auto layoutCache = GraphicsAPI::GetActiveDriver<VulkanDriver>()->layoutCache.get();

        PipelineLayoutKey pipelineKey{};
        m_descriptorSetCount = variant->descriptorSetCount;

        if (variant->descriptorSetCount > 0)
        {
            std::vector<ResourceElement> elements;
            auto* pDescriptorSets = variant->descriptorSets.Get(base);

            for (auto i = 0u; i < variant->descriptorSetCount; ++i)
            {
                DescriptorSetLayoutKey key{};

                auto pDescriptorSet = pDescriptorSets + i;
                auto pDescriptors = pDescriptorSet->descriptors.Get(base);
                elements.clear();

                for (auto j = 0u; j < pDescriptorSet->descriptorCount; ++j)
                {
                    key.counts[j] = pDescriptors[j].count;
                    key.types[j] = EnumConvert::GetDescriptorType(pDescriptors[j].type);
                    elements.emplace_back(pDescriptors[j].type, std::string(pDescriptors[j].name), pDescriptors[j].count);
                }

                key.stageFlags = EnumConvert::GetShaderStageFlags(pDescriptorSet->stageflags);
                m_descriptorSetLayouts[i] = layoutCache->GetSetLayout(key);
                pipelineKey.setlayouts[i] = m_descriptorSetLayouts[i]->layout;
                m_resourceLayouts[i] = ResourceLayout(elements);
            }
        }

        if (variant->constantVariableCount > 0)
        {
            std::vector<ConstantVariable> variables;

            auto pVariables = variant->constantVariables.Get(base);

            for (auto i = 0u; i < variant->constantVariableCount; ++i)
            {
                auto pVariable = pVariables + i;
                variables.emplace_back(pVariable->name, pVariable->size, pVariable->offset, pVariable->stageFlags);
                pipelineKey.pushConstants[i].stageFlags = EnumConvert::GetShaderStageFlags(pVariable->stageFlags);
                pipelineKey.pushConstants[i].offset = pVariable->offset;
                pipelineKey.pushConstants[i].size = pVariable->size;
            }

            m_constantLayout = ConstantBufferLayout(variables);
        }

        m_pipelineLayout = layoutCache->GetPipelineLayout(pipelineKey);
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
                module = nullptr;
            }
        }
    }
}
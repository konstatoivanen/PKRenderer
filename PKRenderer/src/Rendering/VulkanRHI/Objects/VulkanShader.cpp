#include "PrecompiledHeader.h"
#include "VulkanShader.h"
#include "Math/FunctionsMisc.h"
#include "Rendering/VulkanRHI/Utilities/VulkanUtilities.h"
#include "Rendering/VulkanRHI/Utilities/VulkanExtensions.h"
#include "Rendering/VulkanRHI/VulkanDriver.h"
#include "Rendering/GraphicsAPI.h"
#include <PKAssets/PKAssetLoader.h>
#include <vulkan/vk_enum_string_helper.h>

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace VulkanRHI::Utilities;
    using namespace Structs;
    using namespace Services;

    VulkanShader::VulkanShader(void* base, PK::Assets::Shader::PKShaderVariant* variant, const char* name) : 
        m_device(GraphicsAPI::GetActiveDriver<VulkanDriver>()->device),
        m_name(name)
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
            auto stage = EnumConvert::GetShaderStage((ShaderStage)i);
            auto moduleName = std::string(name) + std::string(".") + string_VkShaderStageFlagBits(stage);
            m_modules[i] = new VulkanShaderModule(m_device, stage, spirv, spirvSize, moduleName.c_str());
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

                PK_WARNING_ASSERT(pDescriptorSet->descriptorCount <= PK_MAX_DESCRIPTORS_PER_SET, "Warning: Shader descriptor count exceeds the maximum count per set!");

                for (auto j = 0u; j < pDescriptorSet->descriptorCount; ++j)
                {
                    key.counts[j] = pDescriptors[j].count;
                    key.types[j] = EnumConvert::GetDescriptorType(pDescriptors[j].type);
                    elements.emplace_back(pDescriptors[j].type, std::string(pDescriptors[j].name), pDescriptors[j].writeStageMask, pDescriptors[j].count);
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
        m_type = m_modules[(int)ShaderStage::Compute] != nullptr ? ShaderType::Compute : m_modules[(int)ShaderStage::RayGeneration] ? ShaderType::RayTracing : ShaderType::Graphics;
    }

    VulkanShader::~VulkanShader()
    {
        Dispose();
    }

    void VulkanShader::Dispose()
    {
        auto driver = GraphicsAPI::GetActiveDriver<VulkanDriver>();
        auto fence = driver->GetQueueFenceRef(QueueType::Graphics);

        for (auto& module : m_modules)
        {
            if (module != nullptr)
            {
                driver->disposer->Dispose(module, fence);
                module = nullptr;
            }
        }
    }

    Structs::ShaderBindingTableInfo VulkanShader::GetShaderBindingTableInfo() const
    {
        ShaderBindingTableInfo info{};
        
        auto driver = GraphicsAPI::GetActiveDriver<VulkanDriver>();

        const auto& deviceProperties = driver->physicalDeviceProperties;
        const auto handleSize = deviceProperties.rayTracingProperties.shaderGroupHandleSize;
        const auto handleAlignment = deviceProperties.rayTracingProperties.shaderGroupHandleAlignment;
        const auto tableAlignment = deviceProperties.rayTracingProperties.shaderGroupBaseAlignment;

        info.handleSize = handleSize;
        info.handleSizeAligned = PK::Math::Functions::GetAlignedSize(handleSize, handleAlignment);
        info.tableAlignment = tableAlignment;
        info.totalTableSize = 0u;

        RayTracingShaderGroup currentGroup = RayTracingShaderGroup::MaxCount;

        for (auto i = (uint32_t)ShaderStage::RayGeneration; i < (uint32_t)ShaderStage::MaxCount; ++i)
        {
            if (m_modules[i] == nullptr)
            {
                continue;
            }

            if (PK_SHADER_STAGE_RAYTRACING_GROUP[i] != currentGroup)
            {
                currentGroup = PK_SHADER_STAGE_RAYTRACING_GROUP[i];
                info.totalTableSize = PK::Math::Functions::GetAlignedSize(info.totalTableSize, tableAlignment);
                info.byteOffsets[(uint32_t)currentGroup] = info.totalTableSize;
                info.byteStrides[(uint32_t)currentGroup] = info.handleSizeAligned;
                info.offsets[(uint32_t)currentGroup] = (uint8_t)info.totalHandleCount;
                info.layouts[(uint32_t)currentGroup] = nullptr;
            }

            info.counts[(uint32_t)currentGroup]++;
            info.totalHandleCount++;
            info.totalTableSize += info.handleSizeAligned;
        }

        const uint32_t sbtSize = info.totalTableSize;
        const uint32_t maxSize = sizeof(info.handleData);

        PK_THROW_ASSERT(sbtSize, "SBT has no data!");
        PK_THROW_ASSERT(sbtSize <= maxSize, "SBT is too big to fit to static memory");

        auto pipeline = driver->pipelineCache->GetRayTracingPipeline(this);

        for (auto i = 0u; i < (uint32_t)RayTracingShaderGroup::MaxCount; ++i)
        {
            if (info.counts[i] == 0)
            {
                continue;
            }

            VK_ASSERT_RESULT_CTX(vkGetRayTracingShaderGroupHandlesKHR(driver->device, 
                pipeline->pipeline, 
                info.offsets[i], 
                info.counts[i], 
                info.counts[i] * info.handleSizeAligned, 
                info.handleData + info.byteOffsets[i]), "Failed to get ray tracing shader group handles");
        }

        return info;
    }
}
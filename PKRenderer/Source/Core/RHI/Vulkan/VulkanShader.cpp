#include "PrecompiledHeader.h"
#include <PKAssets/PKAssetLoader.h>
#include "Core/Math/FunctionsMisc.h"
#include "Core/Utilities/FixedString.h"
#include "Core/CLI/Log.h" 
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "VulkanShader.h"

namespace PK
{
    VulkanShader::VulkanShader(VulkanDriver* driver, void* base, PKAssets::PKShaderVariant* variant, const char* name) :
        m_driver(driver),
        m_name(name)
    {
        m_groupSize = { variant->groupSize[0], variant->groupSize[1], variant->groupSize[2] };
        m_stageFlags = (ShaderStageFlags)0u;

        for (auto i = 0u; i < (uint32_t)ShaderStage::MaxCount; ++i)
        {
            m_modules[i] = VK_NULL_HANDLE;

            if (variant->sprivSizes[i] > 0)
            {
                // Manually create these. No point in a wrapper that wastes allocs.
                VkShaderModuleCreateInfo createInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
                createInfo.codeSize = variant->sprivSizes[i];
                createInfo.pCode = reinterpret_cast<uint32_t*>(variant->sprivBuffers[i].Get(base));
                VK_ASSERT_RESULT_CTX(vkCreateShaderModule(m_driver->device, &createInfo, nullptr, &m_modules[i]), "Failed to create shader module!");

                auto vkStage = VulkanEnumConvert::GetShaderStage((ShaderStage)i);
                FixedString128 moduleName({ name, ".", VulkanCStr_VkShaderStageFlagBits(vkStage) });
                VulkanSetObjectDebugName(m_driver->device, VK_OBJECT_TYPE_SHADER_MODULE, (uint64_t)m_modules[i], moduleName.c_str());

                m_stageFlags = m_stageFlags | (ShaderStageFlags)(1u << i);
            }
        }

        if (variant->vertexAttributeCount > 0)
        {
            m_vertexLayout.Clear();

            auto* pVertexAttributes = variant->vertexAttributes.Get(base);

            for (auto i = 0u; i < variant->vertexAttributeCount; ++i)
            {
                auto attribute = &pVertexAttributes[i];
                m_vertexLayout.Add(BufferElement(attribute->type, attribute->name, 1, (byte)attribute->location));
            }
        }

        PipelineLayoutKey pipelineKey{};
        m_descriptorSetCount = variant->descriptorSetCount;

        if (variant->descriptorSetCount > 0)
        {
            auto* pDescriptorSets = variant->descriptorSets.Get(base);

            for (auto i = 0u; i < variant->descriptorSetCount; ++i)
            {
                PK_LOG_INDENT(PK_LOG_LVL_RHI);

                DescriptorSetLayoutKey key{};
                auto pDescriptorSet = pDescriptorSets + i;
                auto pDescriptors = pDescriptorSet->descriptors.Get(base);
                auto& elements = m_resourceLayouts[i];
                elements.ClearFast();

                PK_WARNING_ASSERT(pDescriptorSet->descriptorCount <= PK_RHI_MAX_DESCRIPTORS_PER_SET, "Warning: Shader descriptor count exceeds the maximum count per set!");

                for (auto j = 0u; j < pDescriptorSet->descriptorCount; ++j)
                {
                    key.counts[j] = pDescriptors[j].count;
                    key.types[j] = pDescriptors[j].type;
                    elements.Add(pDescriptors[j].type, pDescriptors[j].name, pDescriptors[j].writeStageMask, pDescriptors[j].count);
                }

                // Cache these so that we can optimize shaders easier based on profiling tools.
                key.stageFlags = VulkanEnumConvert::GetShaderStageFlags(pDescriptorSet->stageflags);
                m_descriptorSetLayouts[i] = m_driver->layoutCache->GetSetLayout(key);
                pipelineKey.setlayouts[i] = m_descriptorSetLayouts[i]->layout;
            }
        }

        if (variant->constantVariableCount > 0)
        {
            auto pVariables = variant->constantVariables.Get(base);

            m_pushConstantLayout.ClearFast();

            for (auto i = 0u; i < variant->constantVariableCount; ++i)
            {
                auto pVariable = pVariables + i;
                auto constant = m_pushConstantLayout.Add();
                constant->name = pVariable->name;
                constant->stageFlags = pVariable->stageFlags;
                constant->size = pVariable->size;
                constant->offset = pVariable->offset;
                pipelineKey.pushConstants[i].stageFlags = VulkanEnumConvert::GetShaderStageFlags(pVariable->stageFlags);
                pipelineKey.pushConstants[i].offset = pVariable->offset;
                pipelineKey.pushConstants[i].size = pVariable->size;
            }
        }

        m_pipelineLayout = m_driver->layoutCache->GetPipelineLayout(pipelineKey);
    }

    VulkanShader::~VulkanShader()
    {
        auto fence = m_driver->GetQueues()->GetLastSubmitFenceRef();

        m_driver->layoutCache->ReleasePipelineLayout(m_pipelineLayout, fence);
        
        for (auto setlayout : m_descriptorSetLayouts)
        {
            if (setlayout != nullptr)
            {
                m_driver->layoutCache->ReleaseSetLayout(setlayout, fence);
            }
        }

        for (auto& module : m_modules)
        {
            if (module != VK_NULL_HANDLE)
            {
                auto deleter = [](void* c, void* v)
                {
                    vkDestroyShaderModule(reinterpret_cast<VkDevice>(c), reinterpret_cast<VkShaderModule>(v), nullptr);
                };

                m_driver->disposer->Dispose(m_driver->device, module, deleter, fence);
                module = VK_NULL_HANDLE;
            }
        }
    }

    ShaderBindingTableInfo VulkanShader::GetShaderBindingTableInfo() const
    {
        ShaderBindingTableInfo info{};

        const auto& deviceProperties = m_driver->physicalDeviceProperties;
        const auto handleSize = deviceProperties.rayTracing.shaderGroupHandleSize;
        const auto handleAlignment = deviceProperties.rayTracing.shaderGroupHandleAlignment;
        const auto tableAlignment = deviceProperties.rayTracing.shaderGroupBaseAlignment;

        info.handleSize = handleSize;
        info.handleSizeAligned = Math::GetAlignedSize(handleSize, handleAlignment);
        info.tableAlignment = tableAlignment;
        info.totalTableSize = 0u;

        RayTracingShaderGroup currentGroup = RayTracingShaderGroup::MaxCount;

        for (auto i = (uint32_t)ShaderStage::RayGeneration; i < (uint32_t)ShaderStage::MaxCount; ++i)
        {
            if (m_modules[i] != VK_NULL_HANDLE)
            {
                if (PK_RHI_SHADER_STAGE_RAYTRACING_GROUP[i] != currentGroup)
                {
                    currentGroup = PK_RHI_SHADER_STAGE_RAYTRACING_GROUP[i];
                    info.totalTableSize = Math::GetAlignedSize(info.totalTableSize, tableAlignment);
                    info.byteOffsets[(uint32_t)currentGroup] = info.totalTableSize;
                    info.byteStrides[(uint32_t)currentGroup] = info.handleSizeAligned;
                    info.offsets[(uint32_t)currentGroup] = (uint8_t)info.totalHandleCount;
                    info.layouts[(uint32_t)currentGroup] = nullptr;
                }

                info.counts[(uint32_t)currentGroup]++;
                info.totalHandleCount++;
                info.totalTableSize += info.handleSizeAligned;
            }
        }

        [[maybe_unused]] const uint32_t sbtSize = info.totalTableSize;
        [[maybe_unused]] const uint32_t maxSize = sizeof(info.handleData);

        PK_DEBUG_THROW_ASSERT(sbtSize, "SBT has no data!");
        PK_DEBUG_THROW_ASSERT(sbtSize <= maxSize, "SBT is too big to fit to static memory");

        auto pipeline = m_driver->pipelineCache->GetRayTracingPipeline(this);

        for (auto i = 0u; i < (uint32_t)RayTracingShaderGroup::MaxCount; ++i)
        {
            if (info.counts[i] > 0)
            {
                VK_ASSERT_RESULT_CTX(vkGetRayTracingShaderGroupHandlesKHR(m_driver->device,
                    pipeline->pipeline,
                    info.offsets[i],
                    info.counts[i],
                    info.counts[i] * info.handleSizeAligned,
                    info.handleData + info.byteOffsets[i]), "Failed to get ray tracing shader group handles");
            }
        }

        return info;
    }
}

#include "PrecompiledHeader.h"
#include <PKAssets/PKAssetLoader.h>
#include <vulkan/vk_enum_string_helper.h>
#include "Math/FunctionsMisc.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanUtilities.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanExtensions.h"
#include "Rendering/RHI/Vulkan/VulkanDriver.h"
#include "VulkanShader.h"

namespace PK::Rendering::RHI::Vulkan::Objects
{
    using namespace PK::Rendering::RHI::Vulkan::Services;
    using namespace PK::Rendering::RHI::Vulkan::Utilities;

    VulkanShader::VulkanShader(void* base, PK::Assets::Shader::PKShaderVariant* variant, const char* name) :
        m_device(RHI::Driver::GetNative<VulkanDriver>()->device),
        m_name(name)
    {
        m_groupSize = { variant->groupSize[0], variant->groupSize[1], variant->groupSize[2] };
        m_stageFlags = (ShaderStageFlags)0u;

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
            m_stageFlags = m_stageFlags | (ShaderStageFlags)(1u << i);
        }

        BufferElement vertexElements[PK::Assets::PK_ASSET_MAX_VERTEX_ATTRIBUTES];
        uint32_t vertexElementCount = 0u;

        for (auto i = 0; i < PK::Assets::PK_ASSET_MAX_VERTEX_ATTRIBUTES; ++i)
        {
            auto attribute = &variant->vertexAttributes[i];

            if (attribute->type != ElementType::Invalid)
            {
                vertexElements[vertexElementCount++] = BufferElement(attribute->type, attribute->name, 1, (byte)attribute->location);
            }
        }

        m_vertexLayout = BufferLayout(vertexElements, vertexElementCount, true);

        auto layoutCache = RHI::Driver::GetNative<VulkanDriver>()->layoutCache.get();

        PipelineLayoutKey pipelineKey{};
        m_descriptorSetCount = variant->descriptorSetCount;

        if (variant->descriptorSetCount > 0)
        {
            auto* pDescriptorSets = variant->descriptorSets.Get(base);

            for (auto i = 0u; i < variant->descriptorSetCount; ++i)
            {
                DescriptorSetLayoutKey key{};

                auto pDescriptorSet = pDescriptorSets + i;
                auto pDescriptors = pDescriptorSet->descriptors.Get(base);
                auto& elements = m_resourceLayouts[i];
                elements.Clear();

                PK_WARNING_ASSERT(pDescriptorSet->descriptorCount <= PK_MAX_DESCRIPTORS_PER_SET, "Warning: Shader descriptor count exceeds the maximum count per set!");

                for (auto j = 0u; j < pDescriptorSet->descriptorCount; ++j)
                {
                    key.counts[j] = pDescriptors[j].count;
                    key.types[j] = EnumConvert::GetDescriptorType(pDescriptors[j].type);
                    elements.Add(pDescriptors[j].type, std::string(pDescriptors[j].name), pDescriptors[j].writeStageMask, pDescriptors[j].count);
                }

                key.stageFlags = EnumConvert::GetShaderStageFlags(pDescriptorSet->stageflags);
                m_descriptorSetLayouts[i] = layoutCache->GetSetLayout(key);
                pipelineKey.setlayouts[i] = m_descriptorSetLayouts[i]->layout;
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
    }

    VulkanShader::~VulkanShader()
    {
        Dispose();
    }

    void VulkanShader::Dispose()
    {
        auto driver = RHI::Driver::GetNative<VulkanDriver>();
        auto fence = driver->GetQueues()->GetFenceRef(QueueType::Graphics);

        for (auto& module : m_modules)
        {
            if (module != nullptr)
            {
                driver->disposer->Dispose(module, fence);
                module = nullptr;
            }
        }
    }

    ShaderBindingTableInfo VulkanShader::GetShaderBindingTableInfo() const
    {
        ShaderBindingTableInfo info{};

        auto driver = RHI::Driver::GetNative<VulkanDriver>();

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
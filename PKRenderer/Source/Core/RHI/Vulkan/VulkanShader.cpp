#include "PrecompiledHeader.h"
#include <PKAssets/PKAssetLoader.h>
#include "Core/Base/Containers/FixedString.h"
#include "Core/RHI/Vulkan/VulkanDriver.h"
#include "Core/CLI/Log.h" 
#include "VulkanShader.h"

namespace PK
{
    VulkanShader::VulkanShader(VulkanDriver* driver, void* base, PKAssets::PKShaderVariant* variant, const char* name) :
        m_driver(driver),
        m_name(name)
    {
        m_groupSize = { variant->groupSize[0], variant->groupSize[1], variant->groupSize[2] };
        m_stageFlags = (ShaderStageFlags)0u;

        for (auto i = 0u; i < (uint32_t)ShaderStage::EnumCount; ++i)
        {
            if (variant->sprivSizes[i])
            {
                m_stageFlags = m_stageFlags | (ShaderStageFlags)(1u << i);
            }
        }

        if ((m_stageFlags & ShaderStageFlags::StagesCompute) != 0u)
        {
            m_bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
        }
        else if ((m_stageFlags & ShaderStageFlags::StagesRayTrace) != 0u)
        {
            m_bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
        }

        if (variant->vertexAttributeCount)
        {
            m_vertexLayout.Clear();
            auto* pVertexAttributes = variant->vertexAttributes.Get(base);

            for (auto i = 0u; i < variant->vertexAttributeCount; ++i)
            {
                auto* attribute = &pVertexAttributes[i];
                m_vertexLayout.Add({ attribute->name, attribute->type, attribute->location });
            }
        }

        m_resourceLayout.ClearFast();
        m_resourceSetSize = 0u;
        m_samplerSetSize = 0u;

        if (variant->descriptorCount)
        {
            PK_WARNING_ASSERT(variant->descriptorCount <= PK_RHI_MAX_DESCRIPTORS_PER_SET, "Warning: Shader descriptor count exceeds the maximum count per set!");
            const auto& descriptorProps = m_driver->physicalDeviceProperties.descriptorHeapDerived;
            auto* pDescriptors = variant->descriptors.Get(base);
            bool hasUnbounded = false;

            for (auto j = 0u; j < variant->descriptorCount; ++j)
            {
                const auto type = pDescriptors[j].type;
                const auto count = pDescriptors[j].count;
                const auto isUnbounded = count >= PK_RHI_MAX_UNBOUNDED_SIZE;
                const auto isImage = type == ShaderResourceType::SamplerTexture || type == ShaderResourceType::Texture || type == ShaderResourceType::Image || type == ShaderResourceType::InputAttachment;
                const auto isBuffer = type == ShaderResourceType::ConstantBuffer || type == ShaderResourceType::StorageBuffer || type == ShaderResourceType::AccelerationStructure;
                const auto isSampler = type == ShaderResourceType::Sampler || type == ShaderResourceType::SamplerTexture;
                const auto resourceStride = !isImage ? !isBuffer ? 0u : descriptorProps.bufferStride : descriptorProps.imageStride;
                const auto samplerStride = isSampler ? descriptorProps.samplerStride : 0u;

                PK_FATAL_ASSERT(isUnbounded || !hasUnbounded, "Fixed-size descriptor binding '%s' (binding %u) cannot follow an unbounded bindless array!", pDescriptors[j].name, j);
                hasUnbounded |= isUnbounded;

                auto& descriptor = m_descriptors[j];
                descriptor.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_AND_BINDING_MAPPING_EXT;
                descriptor.pNext = nullptr;
                descriptor.descriptorSet = 0u;
                descriptor.firstBinding = j;
                descriptor.bindingCount = 1u;
                descriptor.resourceMask = VK_SPIRV_RESOURCE_TYPE_ALL_EXT;

                if (isUnbounded)
                {
                    descriptor.source = VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_CONSTANT_OFFSET_EXT;
                    descriptor.sourceData.constantOffset.heapOffset = type == ShaderResourceType::Sampler ? descriptorProps.samplerHeapOffset : descriptorProps.resourceHeapOffset;
                    descriptor.sourceData.constantOffset.heapArrayStride = type == ShaderResourceType::Sampler ? descriptorProps.samplerStride : descriptorProps.maxResourceStride;
                    continue;
                }

                descriptor.source = VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_PUSH_INDEX_EXT;
                
                m_resourceLayout.Add(type, pDescriptors[j].name, pDescriptors[j].accessMask, pDescriptors[j].writeMask, count);

                if (resourceStride)
                {
                    m_resourceSetSize = math::align(m_resourceSetSize, (uint32_t)resourceStride);
                }

                m_descriptorOffsets[j].resourceOffset = m_resourceSetSize;
                m_descriptorOffsets[j].samplerOffset = m_samplerSetSize;
                m_descriptorOffsets[j].resourceStride = (uint16_t)resourceStride;
                m_descriptorOffsets[j].samplerStride = (uint16_t)samplerStride;

                if (type == ShaderResourceType::Sampler)
                {
                    descriptor.sourceData.pushIndex.heapOffset = m_descriptorOffsets[j].samplerOffset;
                    descriptor.sourceData.pushIndex.pushOffset = PK_VK_PUSH_OFFSET_SAMPLER_HEAP;
                    descriptor.sourceData.pushIndex.heapIndexStride = 1u;
                    descriptor.sourceData.pushIndex.heapArrayStride = samplerStride;
                }
                else
                {
                    descriptor.sourceData.pushIndex.heapOffset = m_descriptorOffsets[j].resourceOffset;
                    descriptor.sourceData.pushIndex.pushOffset = PK_VK_PUSH_OFFSET_RESOURCE_HEAP;
                    descriptor.sourceData.pushIndex.heapIndexStride = 1u;
                    descriptor.sourceData.pushIndex.heapArrayStride = resourceStride;
                    descriptor.sourceData.pushIndex.samplerHeapOffset = m_descriptorOffsets[j].samplerOffset;
                    descriptor.sourceData.pushIndex.samplerPushOffset = PK_VK_PUSH_OFFSET_SAMPLER_HEAP;
                    descriptor.sourceData.pushIndex.samplerHeapIndexStride = 1u;
                    descriptor.sourceData.pushIndex.samplerHeapArrayStride = samplerStride;
                }

                m_resourceSetSize += resourceStride * count;
                m_samplerSetSize += samplerStride * count;
            }

            m_resourceSetSize = math::align(m_resourceSetSize, (uint32_t)descriptorProps.maxResourceStride);
            m_samplerSetSize = math::align(m_samplerSetSize, (uint32_t)descriptorProps.samplerStride);
        }

        m_descriptorLayout.sType = VK_STRUCTURE_TYPE_SHADER_DESCRIPTOR_SET_AND_BINDING_MAPPING_INFO_EXT;
        m_descriptorLayout.pNext = nullptr;
        m_descriptorLayout.mappingCount = variant->descriptorCount;
        m_descriptorLayout.pMappings = variant->descriptorCount ? m_descriptors : nullptr;

        m_pushConstantSize = variant->constantRange;
        m_pushConstantLayout.ClearFast();

        if (variant->constantRange)
        {
            auto* pVariables = variant->constants.Get(base);

            for (auto i = 0u; i < variant->constantCount; ++i)
            {
                auto* constant = m_pushConstantLayout.Add();
                constant->name = pVariables[i].name;
                constant->offset = pVariables[i].offset;
                constant->size = pVariables[i].size;
            }
        }

        if (m_bindPoint == VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR)
        {
            auto stageCount = 0u;
            VkShaderModuleCreateInfo moduleInfos[(uint32_t)ShaderStage::EnumCount]{};
            VkPipelineShaderStageCreateInfo shaderStages[(uint32_t)ShaderStage::EnumCount]{};
            VkRayTracingShaderGroupCreateInfoKHR shaderGroups[(uint32_t)ShaderStage::EnumCount]{};

            for (auto i = (uint32_t)ShaderStage::RayGeneration; i < (uint32_t)ShaderStage::EnumCount; ++i)
            {
                if (variant->sprivSizes[i])
                {
                    const auto stage = (ShaderStage)i;
                    moduleInfos[stageCount].sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                    moduleInfos[stageCount].pNext = &m_descriptorLayout;
                    moduleInfos[stageCount].codeSize = variant->sprivSizes[i];
                    moduleInfos[stageCount].pCode = reinterpret_cast<const uint32_t*>(variant->sprivBuffers[i].Get(base));

                    shaderGroups[stageCount].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
                    shaderGroups[stageCount].type = VulkanEnumConvert::GetRayTracingStageGroupType(stage);
                    shaderGroups[stageCount].generalShader = VK_SHADER_UNUSED_KHR;
                    shaderGroups[stageCount].closestHitShader = VK_SHADER_UNUSED_KHR;
                    shaderGroups[stageCount].anyHitShader = VK_SHADER_UNUSED_KHR;
                    shaderGroups[stageCount].intersectionShader = VK_SHADER_UNUSED_KHR;

                    switch (stage)
                    {
                        case ShaderStage::RayGeneration:
                        case ShaderStage::RayMiss:         shaderGroups[stageCount].generalShader = stageCount; break;
                        case ShaderStage::RayClosestHit:   shaderGroups[stageCount].closestHitShader = stageCount; break;
                        case ShaderStage::RayAnyHit:       shaderGroups[stageCount].anyHitShader = stageCount; break;
                        case ShaderStage::RayIntersection: shaderGroups[stageCount].intersectionShader = stageCount; break;
                        default: break;
                    }

                    shaderStages[stageCount].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                    shaderStages[stageCount].pNext = &moduleInfos[stageCount];
                    shaderStages[stageCount].stage = VulkanEnumConvert::GetShaderStage(stage);
                    shaderStages[stageCount].module = VK_NULL_HANDLE;
                    shaderStages[stageCount].pName = "main";
                    stageCount++;
                }
            }

            VkPipelineCreateFlags2CreateInfo flags2Info{ VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO };
            flags2Info.flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT |
                VK_PIPELINE_CREATE_2_RAY_TRACING_NO_NULL_CLOSEST_HIT_SHADERS_BIT_KHR |
                VK_PIPELINE_CREATE_2_RAY_TRACING_NO_NULL_MISS_SHADERS_BIT_KHR;

            VkRayTracingPipelineCreateInfoKHR pipelineInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
            pipelineInfo.pNext = &flags2Info;
            pipelineInfo.stageCount = stageCount;
            pipelineInfo.pStages = shaderStages;
            pipelineInfo.groupCount = stageCount;
            pipelineInfo.pGroups = shaderGroups;
            pipelineInfo.maxPipelineRayRecursionDepth = 1u;
            pipelineInfo.layout = VK_NULL_HANDLE;

            VK_ASSERT_RESULT_CTX(vkCreateRayTracingPipelinesKHR(m_driver->device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1u, &pipelineInfo, nullptr, &m_rayTracingPipeline), "Failed to create ray tracing pipeline!");
            VulkanSetObjectDebugName(m_driver->device, VK_OBJECT_TYPE_PIPELINE, (uint64_t)m_rayTracingPipeline, name);
        }
        else
        {
            auto stageCount = 0u;
            uint32_t stageIndices[(uint32_t)ShaderStage::RayGeneration]{};
            VkShaderCreateInfoEXT createInfos[(uint32_t)ShaderStage::RayGeneration]{};
            VkShaderEXT createdShaders[(uint32_t)ShaderStage::RayGeneration]{};

            const bool hasNoTask = (m_stageFlags & ShaderStageFlags::MeshTask) == 0u;

            for (auto i = 0u; i < (uint32_t)ShaderStage::RayGeneration; ++i)
            {
                if (variant->sprivSizes[i])
                {
                    const auto stage = (ShaderStage)i;
                    const auto vkStage = VulkanEnumConvert::GetShaderStage(stage);

                    if (stageCount > 0u)
                    {
                        createInfos[stageCount - 1u].nextStage = vkStage;
                    }

                    stageIndices[stageCount] = i;

                    auto& info = createInfos[stageCount++];
                    info.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO_EXT;
                    info.pNext = &m_descriptorLayout;
                    info.flags = VK_SHADER_CREATE_DESCRIPTOR_HEAP_BIT_EXT | (stage == ShaderStage::MeshAssembly && hasNoTask ? VK_SHADER_CREATE_NO_TASK_SHADER_BIT_EXT : 0u);
                    info.stage = vkStage;
                    info.nextStage = 0u;
                    info.codeType = VK_SHADER_CODE_TYPE_SPIRV_EXT;
                    info.codeSize = variant->sprivSizes[i];
                    info.pCode = variant->sprivBuffers[i].Get(base);
                    info.pName = "main";
                }
            }

            VK_ASSERT_RESULT_CTX(vkCreateShadersEXT(m_driver->device, stageCount, createInfos, nullptr, createdShaders), "Failed to create shader objects!");

            for (auto i = 0u; i < stageCount; ++i)
            {
                m_shaders[stageIndices[i]] = createdShaders[i];
                FixedString128 shaderName({ name, ".", VulkanCStr_VkShaderStageFlagBits(createInfos[i].stage) });
                VulkanSetObjectDebugName(m_driver->device, VK_OBJECT_TYPE_SHADER_EXT, (uint64_t)createdShaders[i], shaderName.c_str());
            }
        }
    }

    VulkanShader::~VulkanShader()
    {
        auto fence = m_driver->graph->GetFenceRef();

        if (m_rayTracingPipeline)
        {
            m_driver->disposer->Dispose(m_driver->device, m_rayTracingPipeline, [](void* c, void* v)
            {
                vkDestroyPipeline(static_cast<VkDevice>(c), static_cast<VkPipeline>(v), nullptr);
            },
            fence);
            
            m_rayTracingPipeline = VK_NULL_HANDLE;
        }

        for (auto& shader : m_shaders)
        {
            if (shader != VK_NULL_HANDLE)
            {
                m_driver->disposer->Dispose(m_driver->device, shader, [](void* c, void* v)
                {
                    vkDestroyShaderEXT(static_cast<VkDevice>(c), static_cast<VkShaderEXT>(v), nullptr);
                },
                fence);

                shader = VK_NULL_HANDLE;
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

        info.handleSize = (uint16_t)handleSize;
        info.handleSizeAligned = (uint16_t)math::align(handleSize, handleAlignment);
        info.tableAlignment = (uint16_t)tableAlignment;
        info.totalTableSize = 0u;

        RayTracingShaderGroup currentGroup = RayTracingShaderGroup::EnumCount;

        for (auto i = (uint32_t)ShaderStage::RayGeneration; i < (uint32_t)ShaderStage::EnumCount; ++i)
        {
            if ((m_stageFlags & (1u << i)) != 0u)
            {
                if (PK_RHI_SHADER_STAGE_RAYTRACING_GROUP[i] != currentGroup)
                {
                    currentGroup = PK_RHI_SHADER_STAGE_RAYTRACING_GROUP[i];
                    info.totalTableSize = math::align(info.totalTableSize, (uint16_t)tableAlignment);
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

        PK_DEBUG_FATAL_ASSERT(sbtSize, "SBT has no data!");
        PK_DEBUG_FATAL_ASSERT(sbtSize <= maxSize, "SBT is too big to fit to static memory");

        for (auto i = 0u; i < (uint32_t)RayTracingShaderGroup::EnumCount; ++i)
        {
            if (info.counts[i] > 0)
            {
                VK_ASSERT_RESULT_CTX(vkGetRayTracingShaderGroupHandlesKHR(m_driver->device,
                    m_rayTracingPipeline,
                    info.offsets[i],
                    info.counts[i],
                    info.counts[i] * info.handleSizeAligned,
                    info.handleData + info.byteOffsets[i]), "Failed to get ray tracing shader group handles");
            }
        }

        return info;
    }
}

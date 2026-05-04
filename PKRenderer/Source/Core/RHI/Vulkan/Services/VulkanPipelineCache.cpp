#include "PrecompiledHeader.h"
#include "Core/Utilities/FileIO.h"
#include "Core/CLI/Log.h"
#include "VulkanPipelineCache.h"

namespace PK
{
    VulkanPipelineCache::VulkanPipelineCache(VkDevice device, 
        const VulkanPhysicalDeviceProperties& physicalDeviceProperties, 
        const char* workingDirectory, 
        bool enablePipelineCache,
        uint64_t pruneDelay) :
        m_device(device),
        m_maxOverEstimation(physicalDeviceProperties.conservativeRasterization.maxExtraPrimitiveOverestimationSize),
        m_allowUnderEstimation(physicalDeviceProperties.conservativeRasterization.primitiveUnderestimation),
        m_workingDirectory(workingDirectory),
        m_pipelinePool(),
        m_pruneDelay(pruneDelay)
    {
        if (m_workingDirectory.Length() != 0 && enablePipelineCache)
        {
            void* cacheData = nullptr;
            size_t cacheSize = 0ull;
            FileIO::ReadBinary(FixedString256({ workingDirectory, PIPELINE_CACHE_FILENAME }), false, &cacheData, &cacheSize);
            VkPipelineCacheCreateInfo cacheCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
            cacheCreateInfo.initialDataSize = cacheSize;
            cacheCreateInfo.pInitialData = cacheData;
            VK_ASSERT_RESULT_CTX(vkCreatePipelineCache(device, &cacheCreateInfo, nullptr, &m_pipelineCache), "Failed to create pipeline cache!");
            Memory::Free(cacheData);
        }
    }

    VulkanPipelineCache::~VulkanPipelineCache()
    {
        if (m_pipelineCache != VK_NULL_HANDLE && m_workingDirectory.Length() != 0)
        {
            size_t size = 0ull;
            vkGetPipelineCacheData(m_device, m_pipelineCache, &size, nullptr);
            void* cacheData = Memory::AllocateAligned(size);
            vkGetPipelineCacheData(m_device, m_pipelineCache, &size, cacheData);
            FileIO::WriteBinary(FixedString256({ m_workingDirectory, PIPELINE_CACHE_FILENAME }), false, cacheData, size);
            vkDestroyPipelineCache(m_device, m_pipelineCache, nullptr);
            Memory::Free(cacheData);
        }

        m_pipelinePool.Clear();
        m_vertexPipelines.Clear();
        m_meshPipelines.Clear();
        m_otherPipelines.Clear();
    }

    const VulkanPipeline* VulkanPipelineCache::GetPipeline(const PipelineKey& key)
    {
        auto stageFlags = key.shader->GetStageFlags();

        if ((ShaderStageFlags::StagesGraphics & stageFlags) != 0u)
        {
            return GetGraphicsPipeline(key);
        }

        if ((ShaderStageFlags::StagesCompute & stageFlags) != 0u)
        {
            return GetComputePipeline(key.shader);
        }

        if ((ShaderStageFlags::StagesRayTrace & stageFlags) != 0u)
        {
            return GetRayTracingPipeline(key.shader);
        }

        PK_FATAL_ERROR("Pipeline retrieval failed! Unknown shader type!");
        return nullptr;
    }

    const VulkanPipeline* VulkanPipelineCache::GetGraphicsPipeline(const PipelineKey& key)
    {
        const auto stageFlags = key.shader->GetStageFlags();
        PipelineValue* value = nullptr;

        PK_DEBUG_FATAL_ASSERT(((stageFlags & ShaderStageFlags::StagesMesh) != 0) ^ ((stageFlags & ShaderStageFlags::StagesVertex) != 0) , "Trying to create a graphics pipeline with both vertex & mesh stages!");

        if ((ShaderStageFlags::StagesMesh & stageFlags) != 0u)
        {
            // this doesn't matter for mesh shaders. set it to a default to prevent duplicate pipelines.
            MeshPipelineKey meshKey = { key.shader, key.fixed };
            meshKey.fixed.rasterization.topology = Topology::PointList;
            value = &m_meshPipelines[m_meshPipelines.AddKey(meshKey)].value;
        }

        if ((ShaderStageFlags::StagesVertex & stageFlags) != 0u)
        {
            value = &m_vertexPipelines[m_vertexPipelines.AddKey(key)].value;
        }

        if (value->pipeline == nullptr)
        {
            auto stageCount = 0u;
            VkPipelineShaderStageCreateInfo shaderStages[(uint32_t)ShaderStage::MaxCount];

            for (auto i = 0u; i < (uint32_t)ShaderStage::MaxCount; ++i)
            {
                const auto module = key.shader->GetModule(i);
                const auto stageFlag = (ShaderStageFlags)(1u << i);

                if (module != VK_NULL_HANDLE && (stageFlag & stageFlags) != 0u && (stageFlag & key.fixed.excludeStageMask) == 0u)
                {
                    VkPipelineShaderStageCreateInfo stageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
                    stageInfo.stage = VulkanEnumConvert::GetShaderStage((ShaderStage)i);
                    stageInfo.module = module;
                    stageInfo.pName = PK_RHI_SHADER_ENTRY_POINT_NAME;
                    shaderStages[stageCount++] = stageInfo;
                }
            }

            VkPipelineRenderingCreateInfo renderingInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
            renderingInfo.viewMask = 0u;
            renderingInfo.colorAttachmentCount = 0u;
            renderingInfo.pColorAttachmentFormats = key.fixed.colorFormats;
            renderingInfo.depthAttachmentFormat = key.fixed.depthFormat;
            renderingInfo.stencilAttachmentFormat = VulkanEnumConvert::IsDepthStencilFormat(key.fixed.depthFormat) ? key.fixed.depthFormat : VK_FORMAT_UNDEFINED;
            for (; renderingInfo.colorAttachmentCount < PK_RHI_MAX_RENDER_TARGETS && key.fixed.colorFormats[renderingInfo.colorAttachmentCount] != VK_FORMAT_UNDEFINED; ++renderingInfo.colorAttachmentCount) {}

            VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
            rasterizer.depthClampEnable = key.fixed.rasterization.depthClampEnable;
            rasterizer.rasterizerDiscardEnable = key.fixed.rasterization.rasterizerDiscardEnable;
            rasterizer.polygonMode = VulkanEnumConvert::GetPolygonMode(key.fixed.rasterization.polygonMode);
            rasterizer.lineWidth = key.fixed.rasterization.lineWidth;
            rasterizer.cullMode = VulkanEnumConvert::GetCullMode(key.fixed.rasterization.cullMode);
            rasterizer.frontFace = VulkanEnumConvert::GetFrontFace(key.fixed.rasterization.frontFace);
            rasterizer.depthBiasEnable = key.fixed.rasterization.depthBiasEnable;
            rasterizer.depthBiasConstantFactor = key.fixed.rasterization.depthBiasConstantFactor;
            rasterizer.depthBiasClamp = key.fixed.rasterization.depthBiasClamp;
            rasterizer.depthBiasSlopeFactor = key.fixed.rasterization.depthBiasSlopeFactor;

            VkPipelineRasterizationConservativeStateCreateInfoEXT conservativeRaster{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT };
            conservativeRaster.conservativeRasterizationMode = VulkanEnumConvert::GetRasterMode(key.fixed.rasterization.rasterMode, m_allowUnderEstimation);
            conservativeRaster.extraPrimitiveOverestimationSize = fminf(m_maxOverEstimation, key.fixed.rasterization.overEstimation);
            rasterizer.pNext = key.fixed.rasterization.rasterMode != RasterMode::Default ? &conservativeRaster : nullptr;

            VkPipelineRasterizationLineStateCreateInfo lineRaster{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO };
            lineRaster.lineRasterizationMode = VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH;
            auto rasterPnext = rasterizer.pNext ? &conservativeRaster.pNext : &rasterizer.pNext;
            *rasterPnext = rasterizer.polygonMode == VK_POLYGON_MODE_LINE ? &lineRaster : nullptr;

            VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
            multisampling.sampleShadingEnable = key.fixed.multisampling.sampleShadingEnable;
            multisampling.rasterizationSamples = VulkanEnumConvert::GetSampleCountFlags(key.fixed.multisampling.rasterizationSamples);
            multisampling.minSampleShading = key.fixed.multisampling.minSampleShading;
            multisampling.pSampleMask = nullptr;
            multisampling.alphaToCoverageEnable = key.fixed.multisampling.alphaToCoverageEnable;
            multisampling.alphaToOneEnable = key.fixed.multisampling.alphaToOneEnable;

            VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
            depthStencil.depthTestEnable = key.fixed.depthStencil.depthCompareOp != Comparison::Off;
            depthStencil.depthWriteEnable = key.fixed.depthStencil.depthWriteEnable;
            depthStencil.depthCompareOp = VulkanEnumConvert::GetCompareOp(key.fixed.depthStencil.depthCompareOp);
            depthStencil.depthBoundsTestEnable = key.fixed.depthStencil.depthBoundsTestEnable;
            depthStencil.stencilTestEnable = key.fixed.depthStencil.stencilTestEnable;
            depthStencil.minDepthBounds = key.fixed.depthStencil.minDepthBounds;
            depthStencil.maxDepthBounds = key.fixed.depthStencil.maxDepthBounds;

            VkPipelineColorBlendAttachmentState blendAttachments[PK_RHI_MAX_RENDER_TARGETS];

            for (auto i = 0u; i < renderingInfo.colorAttachmentCount; ++i)
            {
                blendAttachments[i].blendEnable = key.fixed.blending.isBlendEnabled();
                blendAttachments[i].srcColorBlendFactor = VulkanEnumConvert::GetBlendFactor(key.fixed.blending.srcColorFactor, VK_BLEND_FACTOR_ONE);
                blendAttachments[i].dstColorBlendFactor = VulkanEnumConvert::GetBlendFactor(key.fixed.blending.dstColorFactor, VK_BLEND_FACTOR_ZERO);
                blendAttachments[i].colorBlendOp = VulkanEnumConvert::GetBlendOp(key.fixed.blending.colorOp);
                blendAttachments[i].srcAlphaBlendFactor = VulkanEnumConvert::GetBlendFactor(key.fixed.blending.srcAlphaFactor, VK_BLEND_FACTOR_ONE);
                blendAttachments[i].dstAlphaBlendFactor = VulkanEnumConvert::GetBlendFactor(key.fixed.blending.dstAlphaFactor, VK_BLEND_FACTOR_ZERO);
                blendAttachments[i].alphaBlendOp = VulkanEnumConvert::GetBlendOp(key.fixed.blending.alphaOp);
                blendAttachments[i].colorWriteMask = (VkColorComponentFlagBits)key.fixed.blending.colorMask & 0xF;
            }

            VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
            colorBlending.logicOpEnable = key.fixed.blending.isLogicOpEnabled();
            colorBlending.logicOp = VulkanEnumConvert::GetLogicOp(key.fixed.blending.logicOp);
            colorBlending.attachmentCount = renderingInfo.colorAttachmentCount;
            colorBlending.pAttachments = blendAttachments;
            colorBlending.blendConstants[0] = 0.0f;
            colorBlending.blendConstants[1] = 0.0f;
            colorBlending.blendConstants[2] = 0.0f;
            colorBlending.blendConstants[3] = 0.0f;

            VkDynamicState dynamicStates[] =
            {
                VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT,
                VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT,
            };

            VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
            dynamicState.dynamicStateCount = 2;
            dynamicState.pDynamicStates = dynamicStates;

            VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
            VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
            VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };

            VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
            pipelineInfo.pNext = &renderingInfo;
            pipelineInfo.stageCount = stageCount;
            pipelineInfo.pStages = shaderStages;
            pipelineInfo.pVertexInputState = nullptr;
            pipelineInfo.pInputAssemblyState = nullptr;
            pipelineInfo.pViewportState = &viewportState;
            pipelineInfo.pRasterizationState = &rasterizer;
            pipelineInfo.pMultisampleState = &multisampling;
            pipelineInfo.pDepthStencilState = &depthStencil;
            pipelineInfo.pColorBlendState = &colorBlending;
            pipelineInfo.pDynamicState = &dynamicState;
            pipelineInfo.layout = key.shader->GetPipelineLayout()->layout;
            pipelineInfo.renderPass = VK_NULL_HANDLE;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
            pipelineInfo.basePipelineIndex = -1;

            if ((ShaderStageFlags::StagesVertex & stageFlags) != 0u)
            {
                vertexInputInfo.vertexBindingDescriptionCount = 0u;
                vertexInputInfo.vertexAttributeDescriptionCount = 0u;
                vertexInputInfo.pVertexBindingDescriptions = key.vertexStreams;
                vertexInputInfo.pVertexAttributeDescriptions = key.vertexAttributes;

                for (auto i = 0u; i < PK_RHI_MAX_VERTEX_ATTRIBUTES; ++i)
                {
                    vertexInputInfo.vertexBindingDescriptionCount += key.vertexStreams[i].stride != 0;
                    vertexInputInfo.vertexAttributeDescriptionCount += key.vertexAttributes[i].format != VK_FORMAT_UNDEFINED;
                }

                inputAssembly.topology = VulkanEnumConvert::GetTopology(key.fixed.rasterization.topology);
                inputAssembly.primitiveRestartEnable = key.primitiveRestart;
                pipelineInfo.pVertexInputState = &vertexInputInfo;
                pipelineInfo.pInputAssemblyState = &inputAssembly;
            }

            value->pipeline = m_pipelinePool.New(m_device, m_pipelineCache, pipelineInfo, key.shader->GetName());
        }

        value->pruneTick = m_currentPruneTick + m_pruneDelay;
        return value->pipeline;
    }

    const VulkanPipeline* VulkanPipelineCache::GetComputePipeline(const VersionHandle<VulkanShader>& shader)
    {
        auto value = &m_otherPipelines[m_otherPipelines.AddKey(shader)].value;

        if (value->pipeline == nullptr)
        {
            PK_DEBUG_WARNING_ASSERT(value->pipeline == nullptr, "Wack");

            VkComputePipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
            pipelineInfo.stage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
            pipelineInfo.stage.stage = VulkanEnumConvert::GetShaderStage(ShaderStage::Compute);
            pipelineInfo.stage.module = shader->GetModule((uint32_t)ShaderStage::Compute);
            pipelineInfo.stage.pName = PK_RHI_SHADER_ENTRY_POINT_NAME;
            pipelineInfo.layout = shader->GetPipelineLayout()->layout;
            value->pipeline = m_pipelinePool.New(m_device, m_pipelineCache, pipelineInfo, shader->GetName());
        }

        value->pruneTick = m_currentPruneTick + m_pruneDelay;
        return value->pipeline;
    }

    const VulkanPipeline* VulkanPipelineCache::GetRayTracingPipeline(const VersionHandle<VulkanShader>& shader)
    {
        auto value = &m_otherPipelines[m_otherPipelines.AddKey(shader)].value;

        if (value->pipeline == nullptr)
        {
            auto stageCount = 0u;
            auto stageMask = ShaderStageFlags::StagesRayTrace;
            VkPipelineShaderStageCreateInfo shaderStages[(int)ShaderStage::MaxCount]{};
            VkRayTracingShaderGroupCreateInfoKHR shaderGroups[(int)ShaderStage::MaxCount]{};

            for (auto i = 0u; i < (int)ShaderStage::MaxCount; ++i)
            {
                auto stageFlag = (ShaderStageFlags)(1u << i);

                // Skip null modules & modules not viable for this pipeline type
                if (shader->GetModule(i) == VK_NULL_HANDLE || (stageFlag & stageMask) == 0u)
                {
                    continue;
                }

                auto stage = (ShaderStage)i;
                shaderGroups[stageCount].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
                shaderGroups[stageCount].type = VulkanEnumConvert::GetRayTracingStageGroupType(stage);
                shaderGroups[stageCount].generalShader = VK_SHADER_UNUSED_KHR;
                shaderGroups[stageCount].closestHitShader = VK_SHADER_UNUSED_KHR;
                shaderGroups[stageCount].anyHitShader = VK_SHADER_UNUSED_KHR;
                shaderGroups[stageCount].intersectionShader = VK_SHADER_UNUSED_KHR;

                switch (stage)
                {
                    case ShaderStage::RayGeneration:
                    case ShaderStage::RayMiss: shaderGroups[stageCount].generalShader = stageCount; break;
                    case ShaderStage::RayClosestHit: shaderGroups[stageCount].closestHitShader = stageCount; break;
                    case ShaderStage::RayAnyHit: shaderGroups[stageCount].anyHitShader = stageCount; break;
                    case ShaderStage::RayIntersection: shaderGroups[stageCount].intersectionShader = stageCount; break;
                    default: break;
                }

                VkPipelineShaderStageCreateInfo stageInfo{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
                stageInfo.stage = VulkanEnumConvert::GetShaderStage(stage);
                stageInfo.module = shader->GetModule(i);
                stageInfo.pName = PK_RHI_SHADER_ENTRY_POINT_NAME;
                shaderStages[stageCount++] = stageInfo;
            }

            VkRayTracingPipelineCreateInfoKHR pipelineInfo{ VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR };
            pipelineInfo.stageCount = stageCount;
            pipelineInfo.pStages = shaderStages;
            pipelineInfo.groupCount = stageCount;
            pipelineInfo.pGroups = shaderGroups;
            pipelineInfo.maxPipelineRayRecursionDepth = 1;
            pipelineInfo.flags |= VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_CLOSEST_HIT_SHADERS_BIT_KHR;
            pipelineInfo.flags |= VK_PIPELINE_CREATE_RAY_TRACING_NO_NULL_MISS_SHADERS_BIT_KHR;
            pipelineInfo.layout = shader->GetPipelineLayout()->layout;
            value->pipeline = m_pipelinePool.New(m_device, m_pipelineCache, pipelineInfo, shader->GetName());
        }

        value->pruneTick = m_currentPruneTick + m_pruneDelay;
        return value->pipeline;
    }

    void VulkanPipelineCache::Prune()
    {
        m_currentPruneTick++;

        for (int32_t i = m_vertexPipelines.GetCount() - 1; i >= 0; --i)
        {
            auto value = &m_vertexPipelines[i].value;

            if (value->pruneTick < m_currentPruneTick)
            {
                m_pipelinePool.Delete(value->pipeline);
                m_vertexPipelines.RemoveAt(i);
                m_vertexPipelines[m_vertexPipelines.GetCount()].value.pipeline = nullptr;
            }
        }

        for (int32_t i = m_meshPipelines.GetCount() - 1; i >= 0; --i)
        {
            auto value = &m_meshPipelines[i].value;

            if (value->pruneTick < m_currentPruneTick)
            {
                m_pipelinePool.Delete(value->pipeline);
                m_meshPipelines.RemoveAt(i);
                m_meshPipelines[m_meshPipelines.GetCount()].value.pipeline = nullptr;
            }
        }

        for (int32_t i = m_otherPipelines.GetCount() - 1; i >= 0; --i)
        {
            auto value = &m_otherPipelines[i].value;

            if (value->pruneTick < m_currentPruneTick)
            {
                m_pipelinePool.Delete(value->pipeline);
                m_otherPipelines.RemoveAt(i);
                m_otherPipelines[m_otherPipelines.GetCount()].value.pipeline = nullptr;
            }
        }
    }
}

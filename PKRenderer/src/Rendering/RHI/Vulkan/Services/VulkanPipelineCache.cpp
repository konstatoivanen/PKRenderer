#include "PrecompiledHeader.h"
#include "Utilities/FileIOBinary.h"
#include "Core/CLI/Log.h"
#include "VulkanPipelineCache.h"

namespace PK::Rendering::RHI::Vulkan::Services
{
    using namespace PK::Utilities;
    using namespace PK::Rendering::RHI::Vulkan::Objects;

    VulkanPipelineCache::VulkanPipelineCache(VkDevice device, const std::string& workingDirectory, const VulkanPhysicalDeviceProperties& physicalDeviceProperties, uint64_t pruneDelay) :
        m_device(device),
        m_workingDirectory(workingDirectory),
        m_pruneDelay(pruneDelay),
        m_allowUnderEstimation(physicalDeviceProperties.conservativeRasterization.primitiveUnderestimation),
        m_maxOverEstimation(physicalDeviceProperties.conservativeRasterization.maxExtraPrimitiveOverestimationSize),
        m_vertexPipelines(1024),
        m_meshPipelines(1024),
        m_otherPipelines(1024),
        m_pipelinePool()
    {
        if (!workingDirectory.empty())
        {
            void* cacheData = nullptr;
            size_t cacheSize = 0ull;
            FileIO::ReadBinary((workingDirectory + PIPELINE_CACHE_FILENAME).c_str(), &cacheData, &cacheSize);
            VkPipelineCacheCreateInfo cacheCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO };
            cacheCreateInfo.initialDataSize = cacheSize;
            cacheCreateInfo.pInitialData = cacheData;
            VK_ASSERT_RESULT_CTX(vkCreatePipelineCache(device, &cacheCreateInfo, nullptr, &m_pipelineCache), "Failed to create pipeline cache!");
            free(cacheData);
        }
    }

    VulkanPipelineCache::~VulkanPipelineCache()
    {
        if (m_pipelineCache != VK_NULL_HANDLE && !m_workingDirectory.empty())
        {
            size_t size = 0ull;
            vkGetPipelineCacheData(m_device, m_pipelineCache, &size, nullptr);
            void* cacheData = malloc(size);
            vkGetPipelineCacheData(m_device, m_pipelineCache, &size, cacheData);
            FileIO::WriteBinary((m_workingDirectory + PIPELINE_CACHE_FILENAME).c_str(), cacheData, size);
            vkDestroyPipelineCache(m_device, m_pipelineCache, nullptr);
            free(cacheData);
        }

        m_pipelinePool.Clear();
        m_vertexPipelines.Clear();
        m_meshPipelines.Clear();
        m_otherPipelines.Clear();
    }

    const VulkanPipeline* VulkanPipelineCache::GetPipeline(const PipelineKey& key)
    {
        auto stageFlags = key.shader->GetStageFlags();

        if ((ShaderStageFlags::StagesMesh & stageFlags) != 0u)
        {
            MeshPipelineKey meshKey = { key.shader, key.fixedFunctionState, key.renderPass };
            // this doesn't matter for mesh shaders. set it to a default to prevent duplicate pipelines.
            meshKey.fixedFunctionState.rasterization.topology = Topology::TriangleList;
            return GetMeshPipeline(meshKey);
        }

        if ((ShaderStageFlags::StagesVertex & stageFlags) != 0u)
        {
            return GetVertexPipeline(key);
        }

        if ((ShaderStageFlags::StagesCompute & stageFlags) != 0u)
        {
            return GetComputePipeline(key.shader);
        }

        if ((ShaderStageFlags::StagesRayTrace & stageFlags) != 0u)
        {
            return GetRayTracingPipeline(key.shader);
        }

        PK_THROW_ERROR("Pipeline retrieval failed! Unknown shader type!");
    }

    const VulkanPipeline* VulkanPipelineCache::GetVertexPipeline(const PipelineKey& key)
    {
        auto nextPruneTick = m_currentPruneTick + m_pruneDelay;
        uint32_t index = 0u;
        PipelineValue* value = nullptr;

        if (!m_vertexPipelines.AddKey(key, &index))
        {
            value = &m_vertexPipelines.GetValueAt(index);
            value->pruneTick = nextPruneTick;
            return value->pipeline;
        }

        auto stageCount = 0u;
        auto stageMask = ShaderStageFlags::StagesVertex | ShaderStageFlags::Fragment;
        VkPipelineShaderStageCreateInfo shaderStages[(int)ShaderStage::MaxCount];

        for (auto i = 0u; i < (int)ShaderStage::MaxCount; ++i)
        {
            auto stageFlag = (ShaderStageFlags)(1u << i);

            // Skip null modules & modules not viable for this pipeline type
            if (key.shader->GetModule(i) != nullptr &&
                (stageFlag & stageMask) != 0u &&
                (stageFlag & key.fixedFunctionState.excludeStageMask) == 0u)
            {
                shaderStages[stageCount++] = key.shader->GetModule(i)->stageInfo;
            }
        }

        auto bufferCount = 0u;
        auto attributeCount = 0u;

        for (auto i = 0u; i < PK_MAX_VERTEX_ATTRIBUTES; ++i)
        {
            if (key.vertexBuffers[i].stride != 0)
            {
                bufferCount++;
            }

            if (key.vertexAttributes[i].format != VK_FORMAT_UNDEFINED)
            {
                attributeCount++;
            }
        }

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
        vertexInputInfo.vertexBindingDescriptionCount = bufferCount;
        vertexInputInfo.vertexAttributeDescriptionCount = attributeCount;
        vertexInputInfo.pVertexBindingDescriptions = key.vertexBuffers;
        vertexInputInfo.pVertexAttributeDescriptions = key.vertexAttributes;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        inputAssembly.topology = EnumConvert::GetTopology(key.fixedFunctionState.rasterization.topology);
        inputAssembly.primitiveRestartEnable = key.primitiveRestart;

        VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportState.viewportCount = key.fixedFunctionState.viewportCount;
        viewportState.pViewports = nullptr;
        viewportState.scissorCount = key.fixedFunctionState.viewportCount;
        viewportState.pScissors = nullptr;

        VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizer.depthClampEnable = key.fixedFunctionState.rasterization.depthClampEnable;
        rasterizer.rasterizerDiscardEnable = key.fixedFunctionState.rasterization.rasterizerDiscardEnable;
        rasterizer.polygonMode = EnumConvert::GetPolygonMode(key.fixedFunctionState.rasterization.polygonMode);
        rasterizer.lineWidth = key.fixedFunctionState.rasterization.lineWidth;
        rasterizer.cullMode = EnumConvert::GetCullMode(key.fixedFunctionState.rasterization.cullMode);
        rasterizer.frontFace = EnumConvert::GetFrontFace(key.fixedFunctionState.rasterization.frontFace);
        rasterizer.depthBiasEnable = key.fixedFunctionState.rasterization.depthBiasEnable;
        rasterizer.depthBiasConstantFactor = key.fixedFunctionState.rasterization.depthBiasConstantFactor;
        rasterizer.depthBiasClamp = key.fixedFunctionState.rasterization.depthBiasClamp;
        rasterizer.depthBiasSlopeFactor = key.fixedFunctionState.rasterization.depthBiasSlopeFactor;

        VkPipelineRasterizationConservativeStateCreateInfoEXT conservativeRaster{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT };
        conservativeRaster.conservativeRasterizationMode = EnumConvert::GetRasterMode(key.fixedFunctionState.rasterization.rasterMode);
        conservativeRaster.extraPrimitiveOverestimationSize = std::fminf(m_maxOverEstimation, key.fixedFunctionState.rasterization.overEstimation);
        rasterizer.pNext = key.fixedFunctionState.rasterization.rasterMode != RasterMode::Default ? &conservativeRaster : nullptr;

        VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampling.sampleShadingEnable = key.fixedFunctionState.multisampling.sampleShadingEnable;
        multisampling.rasterizationSamples = EnumConvert::GetSampleCountFlags(key.fixedFunctionState.multisampling.rasterizationSamples);
        multisampling.minSampleShading = key.fixedFunctionState.multisampling.minSampleShading;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = key.fixedFunctionState.multisampling.alphaToCoverageEnable;
        multisampling.alphaToOneEnable = key.fixedFunctionState.multisampling.alphaToOneEnable;

        VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencil.depthTestEnable = key.fixedFunctionState.depthStencil.depthCompareOp != Comparison::Off;
        depthStencil.depthWriteEnable = key.fixedFunctionState.depthStencil.depthWriteEnable;
        depthStencil.depthCompareOp = EnumConvert::GetCompareOp(key.fixedFunctionState.depthStencil.depthCompareOp);
        depthStencil.depthBoundsTestEnable = key.fixedFunctionState.depthStencil.depthBoundsTestEnable;
        depthStencil.stencilTestEnable = key.fixedFunctionState.depthStencil.stencilTestEnable;
        depthStencil.minDepthBounds = key.fixedFunctionState.depthStencil.minDepthBounds;
        depthStencil.maxDepthBounds = key.fixedFunctionState.depthStencil.maxDepthBounds;

        VkPipelineColorBlendAttachmentState blendAttachments[PK_MAX_RENDER_TARGETS];

        for (auto i = 0u; i < key.fixedFunctionState.colorTargetCount; ++i)
        {
            blendAttachments[i].blendEnable = key.fixedFunctionState.blending.isBlendEnabled();
            blendAttachments[i].srcColorBlendFactor = EnumConvert::GetBlendFactor(key.fixedFunctionState.blending.srcColorFactor, VK_BLEND_FACTOR_ONE);
            blendAttachments[i].dstColorBlendFactor = EnumConvert::GetBlendFactor(key.fixedFunctionState.blending.dstColorFactor, VK_BLEND_FACTOR_ZERO);
            blendAttachments[i].colorBlendOp = EnumConvert::GetBlendOp(key.fixedFunctionState.blending.colorOp);
            blendAttachments[i].srcAlphaBlendFactor = EnumConvert::GetBlendFactor(key.fixedFunctionState.blending.srcAlphaFactor, VK_BLEND_FACTOR_ONE);
            blendAttachments[i].dstAlphaBlendFactor = EnumConvert::GetBlendFactor(key.fixedFunctionState.blending.dstAlphaFactor, VK_BLEND_FACTOR_ZERO);
            blendAttachments[i].alphaBlendOp = EnumConvert::GetBlendOp(key.fixedFunctionState.blending.alphaOp);
            blendAttachments[i].colorWriteMask = (VkColorComponentFlagBits)key.fixedFunctionState.blending.colorMask & 0xF;
        }

        VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlending.logicOpEnable = key.fixedFunctionState.blending.isLogicOpEnabled();
        colorBlending.logicOp = EnumConvert::GetLogicOp(key.fixedFunctionState.blending.logicOp);
        colorBlending.attachmentCount = key.fixedFunctionState.colorTargetCount;
        colorBlending.pAttachments = blendAttachments;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkDynamicState dynamicStates[] =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineInfo.stageCount = stageCount;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = key.shader->GetPipelineLayout()->layout;
        pipelineInfo.renderPass = key.renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        value = &m_vertexPipelines.GetValueAt(index);
        value->pipeline = m_pipelinePool.New(m_device, m_pipelineCache, pipelineInfo, key.shader->GetName());
        value->pruneTick = nextPruneTick;
        return value->pipeline;
    }

    const VulkanPipeline* VulkanPipelineCache::GetMeshPipeline(const MeshPipelineKey& key)
    {
        auto nextPruneTick = m_currentPruneTick + m_pruneDelay;
        uint32_t index = 0u;
        PipelineValue* value = nullptr;

        if (!m_meshPipelines.AddKey(key, &index))
        {
            value = &m_meshPipelines.GetValueAt(index);
            value->pruneTick = nextPruneTick;
            return value->pipeline;
        }

        auto stageCount = 0u;
        auto stageMask = ShaderStageFlags::StagesMesh | ShaderStageFlags::Fragment;
        VkPipelineShaderStageCreateInfo shaderStages[(int)ShaderStage::MaxCount];

        for (auto i = 0u; i < (int)ShaderStage::MaxCount; ++i)
        {
            auto stageFlag = (ShaderStageFlags)(1u << i);

            // Skip null modules & modules not viable for this pipeline type
            if (key.shader->GetModule(i) != nullptr &&
                (stageFlag & stageMask) != 0u &&
                (stageFlag & key.fixedFunctionState.excludeStageMask) == 0u)
            {
                shaderStages[stageCount++] = key.shader->GetModule(i)->stageInfo;
            }
        }

        VkPipelineViewportStateCreateInfo viewportState{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
        viewportState.viewportCount = key.fixedFunctionState.viewportCount;
        viewportState.pViewports = nullptr;
        viewportState.scissorCount = key.fixedFunctionState.viewportCount;
        viewportState.pScissors = nullptr;

        VkPipelineRasterizationStateCreateInfo rasterizer{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        rasterizer.depthClampEnable = key.fixedFunctionState.rasterization.depthClampEnable;
        rasterizer.rasterizerDiscardEnable = key.fixedFunctionState.rasterization.rasterizerDiscardEnable;
        rasterizer.polygonMode = EnumConvert::GetPolygonMode(key.fixedFunctionState.rasterization.polygonMode);
        rasterizer.lineWidth = key.fixedFunctionState.rasterization.lineWidth;
        rasterizer.cullMode = EnumConvert::GetCullMode(key.fixedFunctionState.rasterization.cullMode);
        rasterizer.frontFace = EnumConvert::GetFrontFace(key.fixedFunctionState.rasterization.frontFace);
        rasterizer.depthBiasEnable = key.fixedFunctionState.rasterization.depthBiasEnable;
        rasterizer.depthBiasConstantFactor = key.fixedFunctionState.rasterization.depthBiasConstantFactor;
        rasterizer.depthBiasClamp = key.fixedFunctionState.rasterization.depthBiasClamp;
        rasterizer.depthBiasSlopeFactor = key.fixedFunctionState.rasterization.depthBiasSlopeFactor;

        VkPipelineRasterizationConservativeStateCreateInfoEXT conservativeRaster{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT };
        conservativeRaster.conservativeRasterizationMode = EnumConvert::GetRasterMode(key.fixedFunctionState.rasterization.rasterMode);
        conservativeRaster.extraPrimitiveOverestimationSize = std::fminf(m_maxOverEstimation, key.fixedFunctionState.rasterization.overEstimation);
        rasterizer.pNext = key.fixedFunctionState.rasterization.rasterMode != RasterMode::Default ? &conservativeRaster : nullptr;

        VkPipelineMultisampleStateCreateInfo multisampling{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        multisampling.sampleShadingEnable = key.fixedFunctionState.multisampling.sampleShadingEnable;
        multisampling.rasterizationSamples = EnumConvert::GetSampleCountFlags(key.fixedFunctionState.multisampling.rasterizationSamples);
        multisampling.minSampleShading = key.fixedFunctionState.multisampling.minSampleShading;
        multisampling.pSampleMask = nullptr;
        multisampling.alphaToCoverageEnable = key.fixedFunctionState.multisampling.alphaToCoverageEnable;
        multisampling.alphaToOneEnable = key.fixedFunctionState.multisampling.alphaToOneEnable;

        VkPipelineDepthStencilStateCreateInfo depthStencil{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        depthStencil.depthTestEnable = key.fixedFunctionState.depthStencil.depthCompareOp != Comparison::Off;
        depthStencil.depthWriteEnable = key.fixedFunctionState.depthStencil.depthWriteEnable;
        depthStencil.depthCompareOp = EnumConvert::GetCompareOp(key.fixedFunctionState.depthStencil.depthCompareOp);
        depthStencil.depthBoundsTestEnable = key.fixedFunctionState.depthStencil.depthBoundsTestEnable;
        depthStencil.stencilTestEnable = key.fixedFunctionState.depthStencil.stencilTestEnable;
        depthStencil.minDepthBounds = key.fixedFunctionState.depthStencil.minDepthBounds;
        depthStencil.maxDepthBounds = key.fixedFunctionState.depthStencil.maxDepthBounds;

        VkPipelineColorBlendAttachmentState blendAttachments[PK_MAX_RENDER_TARGETS];

        for (auto i = 0u; i < key.fixedFunctionState.colorTargetCount; ++i)
        {
            blendAttachments[i].blendEnable = key.fixedFunctionState.blending.isBlendEnabled();
            blendAttachments[i].srcColorBlendFactor = EnumConvert::GetBlendFactor(key.fixedFunctionState.blending.srcColorFactor, VK_BLEND_FACTOR_ONE);
            blendAttachments[i].dstColorBlendFactor = EnumConvert::GetBlendFactor(key.fixedFunctionState.blending.dstColorFactor, VK_BLEND_FACTOR_ZERO);
            blendAttachments[i].colorBlendOp = EnumConvert::GetBlendOp(key.fixedFunctionState.blending.colorOp);
            blendAttachments[i].srcAlphaBlendFactor = EnumConvert::GetBlendFactor(key.fixedFunctionState.blending.srcAlphaFactor, VK_BLEND_FACTOR_ONE);
            blendAttachments[i].dstAlphaBlendFactor = EnumConvert::GetBlendFactor(key.fixedFunctionState.blending.dstAlphaFactor, VK_BLEND_FACTOR_ZERO);
            blendAttachments[i].alphaBlendOp = EnumConvert::GetBlendOp(key.fixedFunctionState.blending.alphaOp);
            blendAttachments[i].colorWriteMask = (VkColorComponentFlagBits)key.fixedFunctionState.blending.colorMask & 0xF;
        }

        VkPipelineColorBlendStateCreateInfo colorBlending{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        colorBlending.logicOpEnable = key.fixedFunctionState.blending.isLogicOpEnabled();
        colorBlending.logicOp = EnumConvert::GetLogicOp(key.fixedFunctionState.blending.logicOp);
        colorBlending.attachmentCount = key.fixedFunctionState.colorTargetCount;
        colorBlending.pAttachments = blendAttachments;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkDynamicState dynamicStates[] =
        {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
        };

        VkPipelineDynamicStateCreateInfo dynamicState{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        dynamicState.dynamicStateCount = 2;
        dynamicState.pDynamicStates = dynamicStates;

        VkGraphicsPipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
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
        pipelineInfo.renderPass = key.renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        value = &m_meshPipelines.GetValueAt(index);
        value->pipeline = m_pipelinePool.New(m_device, m_pipelineCache, pipelineInfo, key.shader->GetName());
        value->pruneTick = nextPruneTick;
        return value->pipeline;
    }

    const VulkanPipeline* VulkanPipelineCache::GetComputePipeline(const VersionHandle<VulkanShader>& shader)
    {
        auto nextPruneTick = m_currentPruneTick + m_pruneDelay;
        uint32_t index = 0u;
        PipelineValue* value = nullptr;

        if (!m_otherPipelines.AddKey(shader, &index))
        {
            value = &m_otherPipelines.GetValueAt(index);
            value->pruneTick = nextPruneTick;
            return value->pipeline;
        }

        VkComputePipelineCreateInfo pipelineInfo{ VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
        pipelineInfo.stage = shader->GetModule((int)ShaderStage::Compute)->stageInfo;
        pipelineInfo.layout = shader->GetPipelineLayout()->layout;
        value = &m_otherPipelines.GetValueAt(index);
        value->pipeline = m_pipelinePool.New(m_device, m_pipelineCache, pipelineInfo, shader->GetName());
        value->pruneTick = nextPruneTick;
        return value->pipeline;
    }

    const VulkanPipeline* VulkanPipelineCache::GetRayTracingPipeline(const PK::Utilities::VersionHandle<Objects::VulkanShader>& shader)
    {
        auto nextPruneTick = m_currentPruneTick + m_pruneDelay;

        uint32_t index = 0u;
        PipelineValue* value = nullptr;

        if (!m_otherPipelines.AddKey(shader, &index))
        {
            value = &m_otherPipelines.GetValueAt(index);
            value->pruneTick = nextPruneTick;
            return value->pipeline;
        }

        auto stageCount = 0u;
        auto stageMask = ShaderStageFlags::StagesRayTrace;
        VkPipelineShaderStageCreateInfo shaderStages[(int)ShaderStage::MaxCount]{};
        VkRayTracingShaderGroupCreateInfoKHR shaderGroups[(int)ShaderStage::MaxCount]{};

        for (auto i = 0u; i < (int)ShaderStage::MaxCount; ++i)
        {
            auto stageFlag = (ShaderStageFlags)(1u << i);

            // Skip null modules & modules not viable for this pipeline type
            if (shader->GetModule(i) == nullptr || (stageFlag & stageMask) == 0u)
            {
                continue;
            }

            auto stage = (ShaderStage)i;
            shaderGroups[stageCount].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
            shaderGroups[stageCount].type = EnumConvert::GetRayTracingStageGroupType(stage);
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
            }

            shaderStages[stageCount++] = shader->GetModule(i)->stageInfo;
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

        value = &m_otherPipelines.GetValueAt(index);
        value->pipeline = m_pipelinePool.New(m_device, m_pipelineCache, pipelineInfo, shader->GetName());
        value->pruneTick = nextPruneTick;
        return value->pipeline;
    }

    void VulkanPipelineCache::Prune()
    {
        m_currentPruneTick++;

        for (int32_t i = m_vertexPipelines.GetCount() - 1; i >= 0; --i)
        {
            auto value = &m_vertexPipelines.GetValueAt(i);

            if (value->pruneTick < m_currentPruneTick)
            {
                m_pipelinePool.Delete(value->pipeline);
                m_vertexPipelines.RemoveAt(i);
            }
        }

        for (int32_t i = m_meshPipelines.GetCount() - 1; i >= 0; --i)
        {
            auto value = &m_meshPipelines.GetValueAt(i);

            if (value->pruneTick < m_currentPruneTick)
            {
                m_pipelinePool.Delete(value->pipeline);
                m_meshPipelines.RemoveAt(i);
            }
        }

        for (int32_t i = m_otherPipelines.GetCount() - 1; i >= 0; --i)
        {
            auto value = &m_otherPipelines.GetValueAt(i);

            if (value->pruneTick < m_currentPruneTick)
            {
                m_pipelinePool.Delete(value->pipeline);
                m_otherPipelines.RemoveAt(i);
            }
        }
    }
}
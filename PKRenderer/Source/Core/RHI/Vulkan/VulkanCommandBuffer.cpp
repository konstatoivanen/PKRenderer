#include "PrecompiledHeader.h"
#include "Core/RHI/Vulkan/VulkanLimits.h"
#include "Core/RHI/Vulkan/VulkanShader.h"
#include "VulkanCommandBuffer.h"

namespace PK
{
    constexpr static const VkShaderStageFlagBits SHADER_STAGES[(uint32_t)ShaderStage::RayGeneration] =
    {
        VK_SHADER_STAGE_VERTEX_BIT,
        VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
        VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
        VK_SHADER_STAGE_GEOMETRY_BIT,
        VK_SHADER_STAGE_TASK_BIT_EXT,
        VK_SHADER_STAGE_MESH_BIT_EXT,
        VK_SHADER_STAGE_FRAGMENT_BIT,
        VK_SHADER_STAGE_COMPUTE_BIT
    };

    void VulkanCommandBuffer::SetShaderState(const VulkanShader* shader, uint16_t excludeStageMask, const void* pushConstants, uint32_t pushConstantSize)
    {
        if (m_shader != shader || m_excludeStageMask != excludeStageMask)
        {
            const auto bindPoint = shader->GetBindPoint();
            const auto* shaders = shader->GetShaderObjects();

            if (bindPoint == VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR)
            {
                vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, shader->GetRayTracingPipeline());
            }
            else if (bindPoint == VK_PIPELINE_BIND_POINT_COMPUTE)
            {
                constexpr auto computeIdx = (uint32_t)ShaderStage::Compute;
                vkCmdBindShadersEXT(m_commandBuffer, 1u, SHADER_STAGES + computeIdx, shaders + computeIdx);
            }
            else
            {
                constexpr auto graphicsCount = (uint32_t)ShaderStage::Compute;
                VkShaderEXT maskedShaders[graphicsCount];

                if (excludeStageMask)
                {
                    for (auto i = 0u; i < graphicsCount; ++i)
                    {
                        maskedShaders[i] = (excludeStageMask & (1u << i)) ? VK_NULL_HANDLE : shaders[i];
                    }
                }

                vkCmdBindShadersEXT(m_commandBuffer, graphicsCount, SHADER_STAGES, excludeStageMask ? maskedShaders : shaders);
            }

            m_shader = shader;
            m_excludeStageMask = excludeStageMask;
        }

        if (m_pushConstants != pushConstants && pushConstantSize)
        {
            VkPushDataInfoEXT pushDataInfo{ VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT };
            pushDataInfo.offset = 0u;
            pushDataInfo.data = { pushConstants, pushConstantSize };
            vkCmdPushDataEXT(m_commandBuffer, &pushDataInfo);
            m_pushConstants = pushConstants;
        }
    }

    void VulkanCommandBuffer::SetRasterState(const VkRenderingInfo* renderingInfo, const VulkanDynamicState* dynamicState)
    {
        if (m_dynamicState != dynamicState)
        {
            const auto* pre = m_dynamicState;
            const auto* cur = dynamicState;

            if (!pre || pre->viewports != cur->viewports)
            {
                vkCmdSetViewportWithCount(m_commandBuffer, cur->viewportCount, cur->viewports);
            }

            if (!pre || pre->scissors != cur->scissors)
            {
                vkCmdSetScissorWithCount(m_commandBuffer, cur->scissorCount, cur->scissors);
            }

            if (!pre || pre->vertexStreams != cur->vertexStreams || pre->vertexAttributes != cur->vertexAttributes)
            {
                vkCmdSetVertexInputEXT(m_commandBuffer, cur->vertexStreamCount, cur->vertexStreams, cur->vertexAttributeCount, cur->vertexAttributes);
            }

            if (!pre || memcmp(&pre->rasterization, &cur->rasterization, sizeof(RasterizationParameters)) != 0)
            {
                const auto& rs = cur->rasterization;
                const auto polyMode = VulkanEnumConvert::GetPolygonMode(rs.polygonMode);

                vkCmdSetCullMode(m_commandBuffer, VulkanEnumConvert::GetCullMode(rs.cullMode));
                vkCmdSetFrontFace(m_commandBuffer, VulkanEnumConvert::GetFrontFace(rs.frontFace));
                vkCmdSetRasterizerDiscardEnable(m_commandBuffer, rs.rasterizerDiscardEnable);
                vkCmdSetLineWidth(m_commandBuffer, rs.lineWidth);
                vkCmdSetDepthClampEnableEXT(m_commandBuffer, rs.depthClampEnable);
                vkCmdSetPrimitiveTopology(m_commandBuffer, VulkanEnumConvert::GetTopology(rs.topology));
                vkCmdSetPrimitiveRestartEnable(m_commandBuffer, VK_FALSE);
                vkCmdSetPolygonModeEXT(m_commandBuffer, polyMode);
                vkCmdSetConservativeRasterizationModeEXT(m_commandBuffer, VulkanEnumConvert::GetRasterMode(rs.rasterMode, true));
                vkCmdSetDepthBiasEnable(m_commandBuffer, rs.depthBiasEnable);

                if (rs.depthBiasEnable)
                {
                    vkCmdSetDepthBias(m_commandBuffer, rs.depthBiasConstantFactor, rs.depthBiasClamp, rs.depthBiasSlopeFactor);
                }

                if (polyMode == VK_POLYGON_MODE_LINE)
                {
                    vkCmdSetLineRasterizationModeEXT(m_commandBuffer, VK_LINE_RASTERIZATION_MODE_RECTANGULAR_SMOOTH);
                }

                if (rs.rasterMode == RasterMode::OverEstimate)
                {
                    vkCmdSetExtraPrimitiveOverestimationSizeEXT(m_commandBuffer, rs.overEstimation);
                }
            }

            if (!pre || memcmp(&pre->depthStencil, &cur->depthStencil, sizeof(DepthStencilParameters)) != 0)
            {
                const auto& ds = cur->depthStencil;

                vkCmdSetDepthTestEnable(m_commandBuffer, ds.depthCompareOp != Comparison::Off);
                vkCmdSetDepthWriteEnable(m_commandBuffer, ds.depthWriteEnable);
                vkCmdSetDepthCompareOp(m_commandBuffer, VulkanEnumConvert::GetCompareOp(ds.depthCompareOp));
                vkCmdSetDepthBoundsTestEnable(m_commandBuffer, ds.depthBoundsTestEnable);
                vkCmdSetStencilTestEnable(m_commandBuffer, ds.stencilTestEnable);

                if (ds.depthBoundsTestEnable)
                {
                    vkCmdSetDepthBounds(m_commandBuffer, ds.minDepthBounds, ds.maxDepthBounds);
                }
            }

            if (!pre || memcmp(&pre->multisampling, &cur->multisampling, sizeof(MultisamplingParameters)) != 0)
            {
                const auto& ms = cur->multisampling;
                const auto samples = VulkanEnumConvert::GetSampleCountFlags(ms.sampleCount);
                const VkSampleMask fullMask[2] = { ~0u, ~0u };
                const VkExtent2D fragmentSize = { math::max(1u, (uint32_t)ms.fragmentSize.x), math::max(1u, (uint32_t)ms.fragmentSize.y) };
                const VkFragmentShadingRateCombinerOpKHR combinerOps[2] =
                {
                    VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,
                    VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR
                };

                vkCmdSetAlphaToCoverageEnableEXT(m_commandBuffer, ms.alphaToCoverageEnable);
                vkCmdSetAlphaToOneEnableEXT(m_commandBuffer, ms.alphaToOneEnable);
                vkCmdSetRasterizationSamplesEXT(m_commandBuffer, samples);
                vkCmdSetSampleMaskEXT(m_commandBuffer, samples, fullMask);
                vkCmdSetFragmentShadingRateKHR(m_commandBuffer, &fragmentSize, combinerOps);
            }

            if (!pre || pre->colorAttachmentCount != cur->colorAttachmentCount || memcmp(&pre->blending, &cur->blending, sizeof(BlendParameters)) != 0)
            {
                const auto& blend = cur->blending;
                vkCmdSetLogicOpEnableEXT(m_commandBuffer, blend.isLogicOpEnabled());

                if (blend.isLogicOpEnabled())
                {
                    vkCmdSetLogicOpEXT(m_commandBuffer, VulkanEnumConvert::GetLogicOp(blend.logicOp));
                }

                if (cur->colorAttachmentCount)
                {
                    VkBool32 blendEnables[PK_RHI_MAX_RENDER_TARGETS]{};
                    VkColorBlendEquationEXT blendEquations[PK_RHI_MAX_RENDER_TARGETS]{};
                    VkColorComponentFlags writeMasks[PK_RHI_MAX_RENDER_TARGETS]{};

                    VkColorBlendEquationEXT equation{};
                    equation.srcColorBlendFactor = VulkanEnumConvert::GetBlendFactor(blend.srcColorFactor, VK_BLEND_FACTOR_ONE);
                    equation.dstColorBlendFactor = VulkanEnumConvert::GetBlendFactor(blend.dstColorFactor, VK_BLEND_FACTOR_ZERO);
                    equation.colorBlendOp = VulkanEnumConvert::GetBlendOp(blend.colorOp);
                    equation.srcAlphaBlendFactor = VulkanEnumConvert::GetBlendFactor(blend.srcAlphaFactor, VK_BLEND_FACTOR_ONE);
                    equation.dstAlphaBlendFactor = VulkanEnumConvert::GetBlendFactor(blend.dstAlphaFactor, VK_BLEND_FACTOR_ZERO);
                    equation.alphaBlendOp = VulkanEnumConvert::GetBlendOp(blend.alphaOp);

                    for (auto i = 0u; i < cur->colorAttachmentCount; ++i)
                    {
                        blendEnables[i] = blend.isBlendEnabled();
                        blendEquations[i] = equation;
                        writeMasks[i] = (VkColorComponentFlagBits)blend.colorMask;
                    }

                    vkCmdSetColorBlendEnableEXT(m_commandBuffer, 0u, cur->colorAttachmentCount, blendEnables);
                    vkCmdSetColorBlendEquationEXT(m_commandBuffer, 0u, cur->colorAttachmentCount, blendEquations);
                    vkCmdSetColorWriteMaskEXT(m_commandBuffer, 0u, cur->colorAttachmentCount, writeMasks);
                }
            }

            m_dynamicState = dynamicState;
        }

        m_renderingInfo = renderingInfo;
        BeginRenderPass();
    }

    void VulkanCommandBuffer::SetVertexBuffers(const VkBindVertexBuffer3InfoKHR* infos, uint32_t count)
    {
        if (count > 0u && (m_vertexBuffers != infos || m_vertexBufferCount != count))
        {
            vkCmdBindVertexBuffers3KHR(m_commandBuffer, 0u, count, infos);
            m_vertexBuffers = infos;
            m_vertexBufferCount = count;
        }
    }

    void VulkanCommandBuffer::SetIndexBuffer(const VkBindIndexBuffer3InfoKHR& info)
    {
        if (m_indexRange.address != info.addressRange.address || m_indexRange.size != info.addressRange.size || m_indexType != info.indexType)
        {
            vkCmdBindIndexBuffer3KHR(m_commandBuffer, &info);
            m_indexRange = info.addressRange;
            m_indexType = info.indexType;
        }
    }

    void VulkanCommandBuffer::PrepareForCommands()
    {
        if (!m_isRecording && m_commandBuffer)
        {
            VkCommandBufferBeginInfo beginInfo{ VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            VK_ASSERT_RESULT(vkBeginCommandBuffer(m_commandBuffer, &beginInfo));

            if ((m_queueFlags & (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT)) != 0u)
            {
                vkCmdBindResourceHeapEXT(m_commandBuffer, &m_resourceHeap);
                vkCmdBindSamplerHeapEXT(m_commandBuffer, &m_samplerHeap);
            }

            m_isRecording = true;
        }
    }

    void VulkanCommandBuffer::BeginRenderPass()
    {
        if (!m_isInRenderPass)
        {
            vkCmdBeginRendering(m_commandBuffer, m_renderingInfo);
            m_isInRenderPass = true;
        }
    }

    void VulkanCommandBuffer::EndRenderPass()
    {
        if (m_isInRenderPass)
        {
            vkCmdEndRendering(m_commandBuffer);
            m_isInRenderPass = false;
        }
    }

    void VulkanCommandBuffer::Barrier(const VkMemoryBarrier2* pMemoryBarrier, const VkImageMemoryBarrier2* pImageTransitions, uint32_t imageTransitionCount)
    {
        EndRenderPass();

        VkMemoryBarrier2 memoryBarrier{};

        if (pMemoryBarrier)
        {
            memoryBarrier = *pMemoryBarrier;
            memoryBarrier.srcAccessMask &= m_queueAccessFlags;
            memoryBarrier.srcStageMask &= m_queueStageFlags;
            memoryBarrier.dstAccessMask &= m_queueAccessFlags;
            memoryBarrier.dstStageMask &= m_queueStageFlags;
        }

        VkDependencyInfo barrier{ VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
        barrier.memoryBarrierCount = pMemoryBarrier ? 1u : 0u;
        barrier.pMemoryBarriers = pMemoryBarrier ? &memoryBarrier : nullptr;
        barrier.imageMemoryBarrierCount = imageTransitionCount;
        barrier.pImageMemoryBarriers = pImageTransitions;
        vkCmdPipelineBarrier2(m_commandBuffer, &barrier);
    }

    void VulkanCommandBuffer::Barrier(const VkImageMemoryBarrier2* imageBarrier)
    {
        Barrier(nullptr, imageBarrier, 1u);
    }

    void VulkanCommandBuffer::QueueInitialize(
        VkCommandBuffer commandBuffer,
        VkQueueFlags queueFlags,
        VkPipelineStageFlags2 queueStageFlags,
        VkAccessFlags2 queueAccessFlags,
        const VkBindHeapInfoEXT& resourceHeap,
        const VkBindHeapInfoEXT& samplerHeap)
    {
        m_commandBuffer = commandBuffer;
        m_resourceHeap = resourceHeap;
        m_samplerHeap = samplerHeap;
        m_queueStageFlags = queueStageFlags;
        m_queueAccessFlags = queueAccessFlags;
        m_queueFlags = queueFlags;
        m_timelineIndex = ~0ull;
        m_shader = nullptr;
        m_excludeStageMask = 0u;
        m_pushConstants = nullptr;
        m_renderingInfo = nullptr;
        m_dynamicState = nullptr;
        m_vertexBuffers = nullptr;
        m_vertexBufferCount = 0u;
        m_indexRange = { 0ull, 0ull };
        m_indexType = VK_INDEX_TYPE_UINT16;
        m_isRecording = false;
        m_isInRenderPass = false;
    }

    bool VulkanCommandBuffer::QueueSubmit(uint64_t timelineIndex)
    {
        if (m_isRecording)
        {
            EndRenderPass();
            VK_ASSERT_RESULT(vkEndCommandBuffer(m_commandBuffer));
            m_timelineIndex = timelineIndex;
            m_isRecording = false;
            return true;
        }

        return false;
    }

    bool VulkanCommandBuffer::QueueFinalize(uint64_t resolvedTimelineIndex)
    {
        if (m_commandBuffer != VK_NULL_HANDLE && m_timelineIndex <= resolvedTimelineIndex)
        {
            m_commandBuffer = VK_NULL_HANDLE;
            m_timelineIndex = 0ull;
            return true;
        }
    
        return false;
    }
}

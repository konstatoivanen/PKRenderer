#include "PrecompiledHeader.h"
#include "VulkanCommandBuffer.h"

namespace PK
{
    void VulkanCommandBuffer::SetPipelineState(const VulkanPipeline* pipeline, const VulkanPipelineLayout* pipelineLayout, VkDeviceSize descriptorSetOffset, const void* pushConstants)
    {
        auto bufferIndex = 0u;

        if (m_pipeline != pipeline)
        {
            vkCmdBindPipeline(m_commandBuffer, pipeline->bindPoint, pipeline->pipeline);
            m_pipeline = pipeline;
            m_descriptorSetOffset = ~0ull;
            m_pushConstants = nullptr;
        }

        if (m_descriptorSetOffset != descriptorSetOffset)
        {
            vkCmdSetDescriptorBufferOffsetsEXT(m_commandBuffer, pipeline->bindPoint, pipelineLayout->layout, 0, 1, &bufferIndex, &descriptorSetOffset);
            m_descriptorSetOffset = descriptorSetOffset;
        }

        if (m_pushConstants != pushConstants)
        {
            vkCmdPushConstants(m_commandBuffer, pipelineLayout->layout, VK_SHADER_STAGE_ALL, 0u, pipelineLayout->pushConstantSize, pushConstants);
            m_pushConstants = pushConstants;
        }
    }

    void VulkanCommandBuffer::SetRasterState(const VkRenderingInfo* renderingInfo, const VulkanDynamicState* dynamicState)
    {
        if (m_dynamicState != dynamicState)
        {
            const auto* pre = m_dynamicState;
            const auto* cur = dynamicState;
            const auto& ds = dynamicState->depthStencil;
            const auto depthTestEnable = ds.depthCompareOp != Comparison::Off;

            if (!pre || pre->viewports != cur->viewports || pre->viewportCount != cur->viewportCount)  vkCmdSetViewportWithCount(m_commandBuffer, cur->viewportCount, cur->viewports);
            if (!pre || pre->scissors != cur->scissors || pre->scissorCount != cur->scissorCount)      vkCmdSetScissorWithCount(m_commandBuffer, cur->scissorCount, cur->scissors);
            if (!pre || pre->cullMode != cur->cullMode)                                                vkCmdSetCullMode(m_commandBuffer, VulkanEnumConvert::GetCullMode(cur->cullMode));
            if (!pre || pre->frontFace != cur->frontFace)                                              vkCmdSetFrontFace(m_commandBuffer, VulkanEnumConvert::GetFrontFace(cur->frontFace));
            if (!pre || pre->rasterizerDiscardEnable != cur->rasterizerDiscardEnable)                  vkCmdSetRasterizerDiscardEnable(m_commandBuffer, cur->rasterizerDiscardEnable);
            if (!pre || pre->lineWidth != dynamicState->lineWidth)                                     vkCmdSetLineWidth(m_commandBuffer, cur->lineWidth);
            if (!pre || pre->depthBiasEnable != cur->depthBiasEnable)                                  vkCmdSetDepthBiasEnable(m_commandBuffer, cur->depthBiasEnable);
            if (!pre || (pre->depthStencil.depthCompareOp != Comparison::Off) != depthTestEnable)      vkCmdSetDepthTestEnable(m_commandBuffer, depthTestEnable);
            if (!pre || pre->depthStencil.depthWriteEnable != ds.depthWriteEnable)                     vkCmdSetDepthWriteEnable(m_commandBuffer, ds.depthWriteEnable);
            if (!pre || pre->depthStencil.depthCompareOp != ds.depthCompareOp)                         vkCmdSetDepthCompareOp(m_commandBuffer, VulkanEnumConvert::GetCompareOp(ds.depthCompareOp));
            if (!pre || pre->depthStencil.depthBoundsTestEnable != ds.depthBoundsTestEnable)           vkCmdSetDepthBoundsTestEnable(m_commandBuffer, ds.depthBoundsTestEnable);
            if (!pre || pre->depthStencil.stencilTestEnable != ds.stencilTestEnable)                   vkCmdSetStencilTestEnable(m_commandBuffer, ds.stencilTestEnable);

            if (cur->depthBiasEnable && (!pre || pre->depthBiasConstantFactor != cur->depthBiasConstantFactor || pre->depthBiasClamp != cur->depthBiasClamp || pre->depthBiasSlopeFactor != cur->depthBiasSlopeFactor))
            {
                vkCmdSetDepthBias(m_commandBuffer, dynamicState->depthBiasConstantFactor, dynamicState->depthBiasClamp, dynamicState->depthBiasSlopeFactor);
            }

            if (ds.depthBoundsTestEnable && (!pre || pre->depthStencil.minDepthBounds != ds.minDepthBounds || pre->depthStencil.maxDepthBounds != ds.maxDepthBounds))
            {
                vkCmdSetDepthBounds(m_commandBuffer, ds.minDepthBounds, ds.maxDepthBounds);
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
        if (m_indexRange.address != info.addressRange.address || 
            m_indexRange.size != info.addressRange.size || 
            m_indexType != info.indexType)
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
                VkDescriptorBufferBindingInfoEXT bindingInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT };
                bindingInfo.address = m_descriptorHeapAddress;
                bindingInfo.usage = VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;
                vkCmdBindDescriptorBuffersEXT(m_commandBuffer, 1, &bindingInfo);
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

    void VulkanCommandBuffer::QueueInitialize(VkCommandBuffer commandBuffer, VkQueueFlags queueFlags, VkPipelineStageFlags2 queueStageFlags, VkAccessFlags2 queueAccessFlags, VkDeviceAddress descriptorHeapAddress)
    {
        m_commandBuffer = commandBuffer;
        m_descriptorHeapAddress = descriptorHeapAddress;
        m_queueStageFlags = queueStageFlags;
        m_queueAccessFlags = queueAccessFlags;
        m_queueFlags = queueFlags;
        m_timelineIndex = ~0ull;
        m_pipeline = nullptr;
        m_descriptorSetOffset = ~0ull;
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
        if (!m_isRecording)
        {
            return false;
        }

        EndRenderPass();
        VK_ASSERT_RESULT(vkEndCommandBuffer(m_commandBuffer));
        m_timelineIndex = timelineIndex;
        m_isRecording = false;
        return true;
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

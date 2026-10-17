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
            vkCmdPushConstants(m_commandBuffer, pipelineLayout->layout, pipelineLayout->pushConstantStageFlags, 0u, pipelineLayout->pushConstantSize, pushConstants);
            m_pushConstants = pushConstants;
        }
    }

    void VulkanCommandBuffer::SetRasterState(const VkRenderingInfo* renderingInfo, VkViewport* viewports, uint32_t viewportCount, VkRect2D* scissors, uint32_t scissorCount)
    {
        if (m_viewports != viewports || m_viewportCount != viewportCount)
        {
            vkCmdSetViewportWithCount(m_commandBuffer, viewportCount, viewports);
            m_viewports = viewports;
            m_viewportCount = viewportCount;
        }

        if (m_scissors != scissors || m_scissorCount != scissorCount)
        {
            vkCmdSetScissorWithCount(m_commandBuffer, scissorCount, scissors);
            m_scissors = scissors;
            m_scissorCount = scissorCount;
        }

        m_renderingInfo = renderingInfo;
        BeginRenderPass();
    }

    void VulkanCommandBuffer::SetVertexBuffers(const VkBuffer* buffers, const VkDeviceSize* offsets, uint32_t count)
    {
        if (count > 0u && (m_vertexBuffers != buffers || m_vertexBufferOffsets != offsets || m_vertexBufferCount != count))
        {
            vkCmdBindVertexBuffers(m_commandBuffer, 0, count, buffers, offsets);
            m_vertexBuffers = buffers;
            m_vertexBufferOffsets = offsets;
            m_vertexBufferCount = count;
        }
    }

    void VulkanCommandBuffer::SetIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType type)
    {
        if (m_indexBuffer != buffer || m_indexOffset != offset || m_indexType != type)
        {
            vkCmdBindIndexBuffer(m_commandBuffer, buffer, offset, type);
            m_indexBuffer = buffer;
            m_indexOffset = offset;
            m_indexType = type;
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
        m_viewports = nullptr;
        m_scissors = nullptr;
        m_viewportCount = 0u;
        m_scissorCount = 0u;
        m_vertexBuffers = nullptr;
        m_vertexBufferOffsets = nullptr;
        m_vertexBufferCount = 0u;
        m_indexBuffer = VK_NULL_HANDLE;
        m_indexOffset = 0ull;
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

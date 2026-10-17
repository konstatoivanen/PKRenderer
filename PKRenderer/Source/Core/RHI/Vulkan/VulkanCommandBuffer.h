#pragma once
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanCommandBuffer
    {
        inline operator VkCommandBuffer() const { return m_commandBuffer; }
        inline bool IsRecording() const { return m_isRecording; }
        inline bool IsExecuting() const { return m_commandBuffer != VK_NULL_HANDLE; }

        void SetPipelineState(const VulkanPipeline* pipeline, const VulkanPipelineLayout* pipelineLayout, VkDeviceSize descriptorSetOffset, const void* pushConstants);
        void SetRasterState(const VkRenderingInfo* renderingInfo, VkViewport* viewports, uint32_t viewportCount, VkRect2D* scissors, uint32_t scissorCount);
        void SetVertexBuffers(const VkBuffer* buffers, const VkDeviceSize* offsets, uint32_t count);
        void SetIndexBuffer(VkBuffer buffer, VkDeviceSize offset, VkIndexType type);

        void PrepareForCommands();
        void BeginRenderPass();
        void EndRenderPass();
        void Barrier(const VkMemoryBarrier2* pMemoryBarrier, const VkImageMemoryBarrier2* pImageTransitions, uint32_t imageTransitionCount);
        void Barrier(const VkImageMemoryBarrier2* imageBarrier);

        void QueueInitialize(VkCommandBuffer commandBuffer, VkQueueFlags queueFlags, VkPipelineStageFlags2 queueStageFlags, VkAccessFlags2 queueAccessFlags, VkDeviceAddress descriptorHeapAddress);
        bool QueueSubmit(uint64_t timelineIndex);
        bool QueueFinalize(uint64_t resolvedTimelineIndex);

    private:
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
        VkDeviceAddress m_descriptorHeapAddress = 0ull;
        VkPipelineStageFlags2 m_queueStageFlags = 0ull;
        VkAccessFlags2 m_queueAccessFlags = 0ull;
        VkQueueFlags m_queueFlags = 0ull;
        uint64_t m_timelineIndex = ~0ull;

        // Shader cmd state
        const VulkanPipeline* m_pipeline = nullptr;
        VkDeviceSize m_descriptorSetOffset = ~0ull;
        const void* m_pushConstants = nullptr;

        // Raster cmd state
        const VkRenderingInfo* m_renderingInfo = nullptr;
        const VkViewport* m_viewports = nullptr;
        const VkRect2D* m_scissors = nullptr;
        uint32_t m_viewportCount = 0u;
        uint32_t m_scissorCount = 0u;

        // Vertex cmd state
        const VkBuffer* m_vertexBuffers = nullptr;
        const VkDeviceSize* m_vertexBufferOffsets = nullptr;
        uint32_t m_vertexBufferCount = 0u;
        VkBuffer m_indexBuffer = VK_NULL_HANDLE;
        VkDeviceSize m_indexOffset = 0ull;
        VkIndexType m_indexType = VK_INDEX_TYPE_UINT16;

        bool m_isRecording = false;
        bool m_isInRenderPass = false;
    };
}

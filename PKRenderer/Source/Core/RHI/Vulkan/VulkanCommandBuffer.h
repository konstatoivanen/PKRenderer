#pragma once
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    struct VulkanCommandBuffer
    {
        inline operator VkCommandBuffer() const { return m_commandBuffer; }
        inline bool IsRecording() const { return m_isRecording; }
        inline bool IsExecuting() const { return m_commandBuffer != VK_NULL_HANDLE; }

        void SetShaderState(const struct VulkanShader* shader, uint16_t excludeStageMask, uint2 descriptorHeapOffsets, const void* pushConstants, uint32_t pushConstantSize);
        void SetRasterState(const VkRenderingInfo* renderingInfo, const VulkanDynamicState* dynamicState);
        void SetVertexBuffers(const VkBindVertexBuffer3InfoKHR* infos, uint32_t count);
        void SetIndexBuffer(const VkBindIndexBuffer3InfoKHR& info);

        void PrepareForCommands();
        void BeginRenderPass();
        void EndRenderPass();
        void Barrier(const VkMemoryBarrier2* pMemoryBarrier, const VkImageMemoryBarrier2* pImageTransitions, uint32_t imageTransitionCount);
        void Barrier(const VkImageMemoryBarrier2* imageBarrier);

        void QueueInitialize(
            VkCommandBuffer commandBuffer,
            VkQueueFlags queueFlags,
            VkPipelineStageFlags2 queueStageFlags,
            VkAccessFlags2 queueAccessFlags,
            const VkBindHeapInfoEXT& resourceHeap,
            const VkBindHeapInfoEXT& samplerHeap);

        bool QueueSubmit(uint64_t timelineIndex);
        bool QueueFinalize(uint64_t resolvedTimelineIndex);

    private:
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
        VkBindHeapInfoEXT m_resourceHeap{};
        VkBindHeapInfoEXT m_samplerHeap{};
        VkPipelineStageFlags2 m_queueStageFlags = 0ull;
        VkAccessFlags2 m_queueAccessFlags = 0ull;
        VkQueueFlags m_queueFlags = 0ull;
        uint64_t m_timelineIndex = ~0ull;

        // Shader cmd state
        const struct VulkanShader* m_shader = nullptr;
        uint16_t m_excludeStageMask = 0u;
        uint2 m_descriptorHeapOffsets = { ~0u, ~0u };
        const void* m_pushConstants = nullptr;

        // Raster cmd state
        const VkRenderingInfo* m_renderingInfo = nullptr;
        const VulkanDynamicState* m_dynamicState = nullptr;

        // Vertex cmd state
        const VkBindVertexBuffer3InfoKHR* m_vertexBuffers = nullptr;
        uint32_t m_vertexBufferCount = 0u;
        VkDeviceAddressRangeKHR m_indexRange = { 0ull, 0ull };
        VkIndexType m_indexType = VK_INDEX_TYPE_UINT16;

        bool m_isRecording = false;
        bool m_isInRenderPass = false;
    };
}

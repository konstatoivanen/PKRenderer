#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/Vulkan/VulkanCommon.h"

namespace PK
{
    class VulkanRenderState;

    struct VulkanCommandBuffer : public RHICommandBuffer
    {
        VulkanCommandBuffer() {}

        FenceRef GetFenceRef() const final;

        void SetRenderTarget(const RenderTargetBinding* bindings, uint32_t count, const uint4& renderArea, uint32_t layers) final;
        void SetViewPorts(const uint4* rects, uint32_t count) final;
        void SetScissors(const uint4* rects, uint32_t count) final;

        void SetShader(const RHIShader* shader) final;
        void SetVertexBuffers(const RHIBuffer** buffers, uint32_t count) final;
        void SetVertexStreams(const VertexStreamElement* elements, uint32_t count) final;
        void SetIndexBuffer(const RHIBuffer* buffer, ElementType indexFormat) final;
        void SetShaderBindingTable(RayTracingShaderGroup group, const RHIBuffer* buffer, size_t offset, size_t stride, size_t size) final;

        void SetStageExcludeMask(const ShaderStageFlags mask) final;
        void SetBlending(const BlendParameters& blend) final;
        void SetRasterization(const RasterizationParameters& rasterization) final;
        void SetDepthStencil(const DepthStencilParameters& depthStencil) final;
        void SetMultisampling(const MultisamplingParameters& multisampling) final;

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) final;
        void DrawIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) final;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) final;
        void DrawIndexedIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) final;
        void DrawMeshTasks(const uint3& dimensions) final;
        void DrawMeshTasksIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) final;
        void DrawMeshTasksIndirectCount(const RHIBuffer* indirectArguments, size_t offset, const RHIBuffer* countBuffer, size_t countOffset, uint32_t maxDrawCount, uint32_t stride) final;
        void Dispatch(const uint3& dimensions) final;
        void DispatchRays(const uint3& dimensions) final;
        
        void Blit(RHITexture* src, RHISwapchain* dst, FilterMode filter) final;
        void Blit(RHISwapchain* src, RHIBuffer* dst) final;
        void Blit(RHITexture* src, RHITexture* dst, const TextureViewRange& srcRange, const TextureViewRange& dstRange, FilterMode filter) final;
        void Blit(const VulkanBindHandle* src, const VulkanBindHandle* dst, const VkImageBlit& blitRegion, FilterMode filter);

        void Clear(RHIBuffer* dst, size_t offset, size_t size, uint32_t value) final;
        void Clear(RHITexture* dst, const TextureViewRange& range, const TextureClearValue& value) final;

        void UpdateBuffer(RHIBuffer* dst, size_t offset, size_t size, void* data) final;
        void CopyBuffer(RHIBuffer* dst, RHIBuffer* src, size_t srcOffset, size_t dstOffset, size_t size) final;
        void* BeginBufferWrite(RHIBuffer* buffer, size_t offset, size_t size) final;
        void EndBufferWrite(RHIBuffer* buffer) final;

        void CopyToTexture(RHITexture* texture, RHIBuffer* buffer, TextureDataRegion* regions, uint32_t regionCount) final;
        void CopyToTexture(RHITexture* texture, const void* data, size_t size, TextureDataRegion* regions, uint32_t regionCount) final;

        void BeginDebugScope(const char* name, const color& color) final;
        void EndDebugScope() final;

        // Vulkan specific interface
        void BuildAccelerationStructures(uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos);
        void CopyAccelerationStructure(const VkCopyAccelerationStructureInfoKHR* pInfo);
        int32_t QueryAccelerationStructureCompactSize(const VulkanRawAccelerationStructure* structure, VulkanQueryPool* pool);
        void TransitionImageLayout(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, const VkImageSubresourceRange& range);
        void PipelineBarrier(const VulkanBarrierInfo& barrier);
        
        // Blits might leave swapchain images in non presentable layouts.
        // This validates swapchain image layout & should be called for the last cmd before present.
        void ValidateSwapchainPresent(RHISwapchain* swapchain);
        bool ResolveBarriers();
        void ValidatePipeline();
        void EndRenderPass();

        void BeginRecord(VkCommandBuffer commandBuffer, VkFence fence, uint16_t queueFamily, VkCommandBufferLevel level, VulkanRenderState* renderState);
        void EndRecord();
        // Called when the command buffer is finished execution.
        void FinishExecution();

        inline void MarkLastCommandStage(VkPipelineStageFlags stage) { m_lastCommandStage = stage; }
        inline bool IsActive() const { return m_commandBuffer != VK_NULL_HANDLE; }
        inline VkCommandBuffer& GetCommandBuffer() { return m_commandBuffer; }
        inline VkFence& GetFence() { return m_fence; }
        inline VkPipelineStageFlags GetLastCommandStage() { return m_lastCommandStage; }
        
        private:
            VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
            VkFence m_fence = VK_NULL_HANDLE;
            uint16_t m_queueFamily = 0u;
            VkCommandBufferLevel m_level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            VulkanRenderState* m_renderState = nullptr;
            
            uint64_t m_invocationIndex = 0ull;
            VkPipelineStageFlags m_lastCommandStage = 0u;
            bool m_isInActiveRenderPass = false;
    };
}

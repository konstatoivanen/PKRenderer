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
        inline bool IsActive() const { return m_commandBuffer != VK_NULL_HANDLE; }
        inline VkCommandBuffer& GetNative() { return m_commandBuffer; }
        inline VkPipelineStageFlags GetLastCommandStage() { return m_lastCommandStage; }
        inline const VkFence& GetFence() { return m_fence; }
        inline void Initialize(VkFence fence, uint16_t queueFamily, VkPipelineStageFlags capabilities) 
        {
            m_fence = fence;
            m_queueFamily = queueFamily;
            m_capabilityFlags = capabilities;
        }
        inline void Release() { m_commandBuffer = VK_NULL_HANDLE; ++m_invocationIndex; }

        void SetRenderTarget(const uint3& resolution) final;
        void SetRenderTarget(RHITexture* const* renderTargets, RHITexture* const* resolveTargets, const TextureViewRange* ranges, uint32_t count) final;
        void SetViewPorts(const uint4* rects, uint32_t count) final;
        void SetScissors(const uint4* rects, uint32_t count) final;
        void SetUnorderedOverlap(bool value) final { m_isInUnorderedOverlap = value; }

        void SetShader(const RHIShader* shader) final;
        void SetVertexBuffers(const RHIBuffer** buffers, uint32_t count) final;
        void SetVertexStreams(const VertexStreamElement* elements, uint32_t count) final;
        void SetIndexBuffer(const RHIBuffer* buffer, size_t offset, ElementType indexFormat) final;
        void SetShaderBindingTable(RayTracingShaderGroup group, const RHIBuffer* buffer, size_t offset, size_t stride, size_t size) final;

        void ClearColor(const color& color, uint32_t index) final;
        void ClearDepth(float depth, uint32_t stencil) final;
        void DiscardColor(uint32_t index) final;
        void DiscardDepth() final;

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
        
        void Blit(RHITexture* src, RHIWindow* dst, FilterMode filter) final;
        void Blit(RHIWindow* src, RHIBuffer* dst) final;
        void Blit(RHITexture* src, RHITexture* dst, const TextureViewRange& srcRange, const TextureViewRange& dstRange, FilterMode filter) final;
        void Blit(const VulkanBindHandle* src, const VulkanBindHandle* dst, const VkImageBlit& blitRegion, FilterMode filter);

        void Clear(RHIBuffer* dst, size_t offset, size_t size, uint32_t value) final;
        void Clear(RHITexture* dst, const TextureViewRange& range, const uint4& value) final;

        void UpdateBuffer(RHIBuffer* dst, size_t offset, size_t size, void* data) final;
        void* BeginBufferWrite(RHIBuffer* buffer, size_t offset, size_t size) final;
        void EndBufferWrite(RHIBuffer* buffer) final;

        void UploadTexture(RHITexture* texture, const void* data, size_t size, TextureUploadRange* ranges, uint32_t rangeCount) final;
        void UploadTexture(RHITexture* texture, const void* data, size_t size, uint32_t level, uint32_t layer) final;

        void BeginDebugScope(const char* name, const color& color) final;
        void EndDebugScope() final;

        // Vulkan specific interface
        void BuildAccelerationStructures(uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos);
        void CopyAccelerationStructure(const VkCopyAccelerationStructureInfoKHR* pInfo);
        uint32_t QueryAccelerationStructureCompactSize(const VulkanRawAccelerationStructure* structure, VulkanQueryPool* pool);
        void TransitionImageLayout(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, const VkImageSubresourceRange& range);
        void PipelineBarrier(const VulkanBarrierInfo& barrier);
        
        // Blits might leave window images in non presentable layouts.
        // This validates window image layout & should be called for the last cmd before present.
        void ValidateWindowPresent(RHIWindow* window);
        bool ResolveBarriers();
        void ValidatePipeline();
        void EndRenderPass();
        void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferLevel level, VulkanRenderState* renderState);
        void EndCommandBuffer();
        inline void MarkLastCommandStage(VkPipelineStageFlags stage) { m_lastCommandStage = stage; }
        
        private:
            VkFence m_fence = VK_NULL_HANDLE;
            VkPipelineStageFlags m_capabilityFlags = 0u;
            VkPipelineStageFlags m_lastCommandStage = 0u;
            uint16_t m_queueFamily = 0u;

            VulkanRenderState* m_renderState = nullptr;
            VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
            VkCommandBufferLevel m_level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            uint64_t m_invocationIndex = 0ull;
            bool m_isInActiveRenderPass = false;
            bool m_isInUnorderedOverlap = false;
    };
}
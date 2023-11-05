#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "Rendering/RHI/Vulkan/Objects/VulkanRenderState.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanEnumConversion.h"
#include "Rendering/RHI/Vulkan/Utilities/VulkanStructs.h"

namespace PK::Rendering::RHI::Vulkan::Objects
{
    // Forgive me for this
    using namespace PK::Math;
    using namespace PK::Rendering::RHI::Objects;

    struct VulkanCommandBuffer : public RHI::Objects::CommandBuffer
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
        void SetRenderTarget(Texture* const* renderTargets, Texture* const* resolveTargets, const TextureViewRange* ranges, uint32_t count) final;
        void SetViewPorts(const uint4* rects, uint32_t count) final;
        void SetScissors(const uint4* rects, uint32_t count) final;

        void SetShader(const Shader* shader, int32_t variantIndex = -1) final;
        void SetVertexBuffers(const Buffer** buffers, uint32_t count) final;
        void SetIndexBuffer(const Buffer* buffer, size_t offset) final;
        void SetShaderBindingTable(RayTracingShaderGroup group, const Buffer* buffer, size_t offset, size_t stride, size_t size) final;

        inline void ClearColor(const color& color, uint32_t index) final { m_renderState->ClearColor(color, index); }
        inline void ClearDepth(float depth, uint32_t stencil) final { m_renderState->ClearDepth(depth, stencil); }
        inline void DiscardColor(uint32_t index) final { m_renderState->DiscardColor(index); }
        inline void DiscardDepth() final { m_renderState->DiscardDepth(); }

        inline void SetBlending(const BlendParameters& blend) final { m_renderState->SetBlending(blend); }
        inline void SetRasterization(const RasterizationParameters& rasterization) final { m_renderState->SetRasterization(rasterization); }
        inline void SetDepthStencil(const DepthStencilParameters& depthStencil) final { m_renderState->SetDepthStencil(depthStencil); }
        inline void SetMultisampling(const MultisamplingParameters& multisampling) final { m_renderState->SetMultisampling(multisampling); }

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) final;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) final;
        void DrawIndexedIndirect(const Buffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) final;
        void Dispatch(Math::uint3 dimensions) final;
        void DispatchRays(Math::uint3 dimensions) final;
        
        void Blit(Texture* src, Window* dst, FilterMode filter) final;
        void Blit(Window* src, Buffer* dst) final;
        void Blit(Texture* src, Texture* dst, const TextureViewRange& srcRange, const TextureViewRange& dstRange, FilterMode filter) final;
        void Blit(const VulkanBindHandle* src, const VulkanBindHandle* dst, const VkImageBlit& blitRegion, FilterMode filter);

        void Clear(Buffer* dst, size_t offset, size_t size, uint32_t value) final;
        void Clear(Texture* dst, const TextureViewRange& range, const uint4& value) final;

        void* BeginBufferWrite(Buffer* buffer, size_t offset, size_t size) final;
        void EndBufferWrite(Buffer* buffer) final;

        void UploadTexture(Texture* texture, const void* data, size_t size, ImageUploadRange* ranges, uint32_t rangeCount) final;
        void UploadTexture(Texture* texture, const void* data, size_t size, uint32_t level, uint32_t layer) final;

        void BeginDebugScope(const char* name, const Math::color& color) final;
        void EndDebugScope() final;

        // Vulkan specific interface
        void BuildAccelerationStructures(uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos);
        void CopyAccelerationStructure(const VkCopyAccelerationStructureInfoKHR* pInfo);
        uint32_t QueryAccelerationStructureCompactSize(const VulkanRawAccelerationStructure* structure, VulkanQueryPool* pool);
        void TransitionImageLayout(VkImage image, VkImageLayout srcLayout, VkImageLayout dstLayout, const VkImageSubresourceRange& range);
        void PipelineBarrier(const VulkanBarrierInfo& barrier);
        
        // Blits might leave window images in non presentable layouts.
        // This validates window image layout & should be called for the last cmd before present.
        void ValidateWindowPresent(Window* window);
        bool ResolveBarriers();
        void ValidatePipeline();
        void EndRenderPass();
        void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferLevel level, Objects::VulkanRenderState* renderState);
        void EndCommandBuffer();
        inline void MarkLastCommandStage(VkPipelineStageFlags stage) { m_lastCommandStage = stage; }
        
        private:
            VkFence m_fence = VK_NULL_HANDLE;
            VkPipelineStageFlags m_capabilityFlags = 0u;
            VkPipelineStageFlags m_lastCommandStage = 0u;
            uint16_t m_queueFamily = 0u;

            Objects::VulkanRenderState* m_renderState = nullptr;
            VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
            VkCommandBufferLevel m_level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            uint64_t m_invocationIndex = 0ull;
            bool m_isInActiveRenderPass = false;
    };
}
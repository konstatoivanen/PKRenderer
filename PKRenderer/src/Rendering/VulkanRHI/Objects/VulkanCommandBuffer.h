#pragma once
#include "PrecompiledHeader.h"
#include "Rendering/Objects/CommandBuffer.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Objects/VulkanRenderState.h"
#include "Core/NoCopy.h"
#include "Utilities/Ref.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    using namespace PK::Rendering::VulkanRHI;
    using namespace PK::Rendering::Objects;
    using namespace PK::Utilities;

    struct VulkanCommandBuffer : public CommandBuffer
    {
        VulkanCommandBuffer() {}

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        uint64_t invocationIndex = 0;
        VulkanRenderState* renderState = nullptr;
        Ref<VulkanFence> fence = nullptr;
        bool isInActiveRenderPass = false;

        inline bool IsActive() const { return commandBuffer != VK_NULL_HANDLE; }

        inline VulkanExecutionGate GetOnCompleteGate() const { return { invocationIndex, &invocationIndex }; }

        void SetRenderTarget(Texture** renderTargets, Texture** resolveTargets, const TextureViewRange* ranges, uint32_t count) override final;
        void SetViewPort(uint4 rect, float mindepth, float maxdepth, uint index = 0) override final;
        void SetScissor(uint4 rect, uint index = 0) override final;

        void SetShader(const Shader* shader, int variantIndex = -1) override final;
        void SetVertexBuffers(const Buffer** buffers, uint count) override final;
        void SetIndexBuffer(const Buffer* buffer, size_t offset) override final;
        void SetBuffer(uint32_t nameHashId, const Buffer* buffer) override final;
        void SetTexture(uint32_t nameHashId, Texture* texture, const TextureViewRange& range) override final;
        void SetImage(uint32_t nameHashId, Texture* texture, const TextureViewRange& range) override final;
        void SetConstant(uint32_t nameHashId, const void* data, uint32_t size) override final;
        void SetKeyword(uint32_t nameHashId, bool value) override final;

        inline void ClearColor(const color& color, uint32_t index) override final { renderState->ClearColor(color, index); }
        inline void ClearDepth(float depth, uint32_t stencil) override final { renderState->ClearDepth(depth, stencil); }
        inline void DiscardColor(uint32_t index) override final { renderState->DiscardColor(index); }
        inline void DiscardDepth() override final { renderState->DiscardDepth(); }

        inline void SetBlending(const BlendParameters& blend) override final { renderState->SetBlending(blend); }
        inline void SetRasterization(const RasterizationParameters& rasterization) override final { renderState->SetRasterization(rasterization); }
        inline void SetDepthStencil(const DepthStencilParameters& depthStencil) override final { renderState->SetDepthStencil(depthStencil); }
        inline void SetMultisampling(const MultisamplingParameters& multisampling) override final { renderState->SetMultisampling(multisampling); }

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override final;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override final;
        void Dispatch(uint3 groupCount) override final;
        
        void Blit(Texture* src, Window* dst, uint32_t dstLevel, uint32_t dstLayer, FilterMode filter) override final;
        void Blit(Texture* src, Texture* dst, uint32_t srcLevel, uint32_t dstLevel, uint32_t srcLayer, uint32_t dstLayer, FilterMode filter) override final;
        void Blit(const VulkanRenderTarget& src, const VulkanRenderTarget& dst, uint32_t srcLevel, uint32_t dstLevel, uint32_t srcLayer, uint32_t dstLayer, FilterMode filter);

        void Barrier(const Texture* texture, const TextureViewRange& range, const Buffer* buffer, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags) override final;

        // Vulkan specific interface
        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) const;
        void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) const;
        void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, const VkExtent3D& extent, uint32_t level, uint32_t layer) const;
        void TransitionImageLayout(const VulkanLayoutTransition& transition);
        void PipelineBarrier(VkPipelineStageFlags srcStageMask,
                             VkPipelineStageFlags dstStageMask,
                             VkDependencyFlags dependencyFlags,
                             uint32_t memoryBarrierCount,
                             const VkMemoryBarrier* pMemoryBarriers,
                             uint32_t bufferMemoryBarrierCount,
                             const VkBufferMemoryBarrier* pBufferMemoryBarriers,
                             uint32_t imageMemoryBarrierCount,
                             const VkImageMemoryBarrier* pImageMemoryBarriers);

        void ValidatePipeline();
        void EndRenderPass();
    };
}
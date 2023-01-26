#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/Ref.h"
#include "Rendering/Objects/CommandBuffer.h"
#include "Rendering/VulkanRHI/Utilities/VulkanEnumConversion.h"
#include "Rendering/VulkanRHI/Utilities/VulkanStructs.h"
#include "Rendering/VulkanRHI/Objects/VulkanRenderState.h"

namespace PK::Rendering::VulkanRHI::Objects
{
    // Forgive me for this
    using namespace PK::Math;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::Structs;

    struct VulkanCommandBuffer : public CommandBuffer
    {
        using CommandBuffer::Barrier;

        VulkanCommandBuffer() {}

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        uint64_t invocationIndex = 0;
        Objects::VulkanRenderState* renderState = nullptr;
        PK::Utilities::Ref<VulkanFence> fence = nullptr;
        bool isInActiveRenderPass = false;

        inline bool IsActive() const { return commandBuffer != VK_NULL_HANDLE; }
        ExecutionGate GetOnCompleteGate() const override final { return { invocationIndex, &invocationIndex }; }

        void SetRenderTarget(const uint3& resolution) override final;
        void SetRenderTarget(Texture** renderTargets, Texture** resolveTargets, const TextureViewRange* ranges, uint32_t count) override final;
        void SetViewPorts(const uint4* rects, uint32_t count) override final;
        void SetScissors(const uint4* rects, uint32_t count) override final;

        void SetShader(const Shader* shader, int32_t variantIndex = -1) override final;
        void SetVertexBuffers(const Buffer** buffers, uint32_t count) override final;
        void SetIndexBuffer(const Buffer* buffer, size_t offset) override final;
        void SetBuffer(uint32_t nameHashId, Buffer* buffer, const IndexRange& range) override final;
        void SetTexture(uint32_t nameHashId, Texture* texture, const TextureViewRange& range) override final;
        void SetBufferArray(uint32_t nameHashId, BindArray<Buffer>* bufferArray) override final;
        void SetTextureArray(uint32_t nameHashId, BindArray<Texture>* textureArray) override final;
        void SetImage(uint32_t nameHashId, Texture* texture, const TextureViewRange& range) override final;
        void SetAccelerationStructure(uint32_t nameHashId, AccelerationStructure* structure) override final;
        void SetShaderBindingTable(Structs::RayTracingShaderGroup group, const Buffer* buffer, size_t offset, size_t stride, size_t size) override final;
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
        void DrawIndexedIndirect(const Buffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) override final;
        void Dispatch(uint3 groupCount) override final;
        void DispatchRays(Math::uint3 dimensions) override final;
        
        void Blit(Texture* src, Core::Window* dst, FilterMode filter) override final;
        void Blit(Core::Window* src, Buffer* dst) override final;
        void Blit(Texture* src, Texture* dst, const Structs::TextureViewRange& srcRange, const Structs::TextureViewRange& dstRange, FilterMode filter) override final;
        void Blit(const VulkanBindHandle* src, const VulkanBindHandle* dst, uint32_t srcLevel, uint32_t dstLevel, uint32_t srcLayer, uint32_t dstLayer, FilterMode filter, bool flipVertical = false);

        void Clear(Buffer* dst, size_t offset, size_t size, uint32_t value) override final;
        void Clear(Texture* dst, const TextureViewRange& range, const uint4& value) override final;

        void Barrier(const Texture* texture, const TextureViewRange& range, const Buffer* buffer, size_t offset, size_t size, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags) override final;

        void* BeginBufferWrite(Buffer* buffer, size_t offset, size_t size) override final;
        void EndBufferWrite(Buffer* buffer) override final;

        void BeginDebugScope(const char* name, const Math::color& color) override final;
        void EndDebugScope() override final;

        // Vulkan specific interface
        void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, uint32_t regionCount, const VkBufferCopy* pRegions) const;
        void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions) const;
        void CopyBufferToImage(VkBuffer srcBuffer, VkImage dstImage, const VkExtent3D& extent, uint32_t level, uint32_t layer) const;
        void BuildAccelerationStructures(uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos);
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
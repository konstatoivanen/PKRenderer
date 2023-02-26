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
        VulkanCommandBuffer() {}

        FenceRef GetFenceRef() const override final;
        inline bool IsActive() const { return m_commandBuffer != VK_NULL_HANDLE; }
        inline VkCommandBuffer& GetNative() { return m_commandBuffer; }
        inline const VkFence& GetFence() { return m_fence; }
        inline void Initialize(VkFence fence, uint16_t queueFamily, VkPipelineStageFlags capabilities) 
        {
            m_fence = fence;
            m_queueFamily = queueFamily;
            m_capabilityFlags = capabilities;
        }
        inline void Release() { m_commandBuffer = VK_NULL_HANDLE; ++m_invocationIndex; }

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

        inline void ClearColor(const color& color, uint32_t index) override final { m_renderState->ClearColor(color, index); }
        inline void ClearDepth(float depth, uint32_t stencil) override final { m_renderState->ClearDepth(depth, stencil); }
        inline void DiscardColor(uint32_t index) override final { m_renderState->DiscardColor(index); }
        inline void DiscardDepth() override final { m_renderState->DiscardDepth(); }

        inline void SetBlending(const BlendParameters& blend) override final { m_renderState->SetBlending(blend); }
        inline void SetRasterization(const RasterizationParameters& rasterization) override final { m_renderState->SetRasterization(rasterization); }
        inline void SetDepthStencil(const DepthStencilParameters& depthStencil) override final { m_renderState->SetDepthStencil(depthStencil); }
        inline void SetMultisampling(const MultisamplingParameters& multisampling) override final { m_renderState->SetMultisampling(multisampling); }

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

        void* BeginBufferWrite(Buffer* buffer, size_t offset, size_t size) override final;
        void EndBufferWrite(Buffer* buffer) override final;

        void UploadTexture(Texture* texture, const void* data, size_t size, Structs::ImageUploadRange* ranges, uint32_t rangeCount) override final;
        void UploadTexture(Texture* texture, const void* data, size_t size, uint32_t level, uint32_t layer) override final;

        void BeginDebugScope(const char* name, const Math::color& color) override final;
        void EndDebugScope() override final;

        // Vulkan specific interface
        void BuildAccelerationStructures(uint32_t infoCount, const VkAccelerationStructureBuildGeometryInfoKHR* pInfos, const VkAccelerationStructureBuildRangeInfoKHR* const* ppBuildRangeInfos);
        void TransitionImageLayout(const VulkanLayoutTransition& transition);
        void PipelineBarrier(const VulkanBarrierInfo& barrier);
        
        bool ResolveBarriers();
        void ValidatePipeline();
        void EndRenderPass();
        void BeginCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferLevel level, Objects::VulkanRenderState* renderState);
        void EndCommandBuffer(VulkanBarrierInfo* transferBarrier);
        
        private:
            VkFence m_fence = VK_NULL_HANDLE;
            VkPipelineStageFlags m_capabilityFlags = 0u;
            uint16_t m_queueFamily = 0u;

            Objects::VulkanRenderState* m_renderState = nullptr;
            VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
            VkCommandBufferLevel m_level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            uint64_t m_invocationIndex = 0ull;
            bool m_isInActiveRenderPass = false;
    };
}
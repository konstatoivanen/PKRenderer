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

    struct VulkanCommandBuffer : public VulkanRawCommandBuffer, public CommandBuffer
    {
        VulkanCommandBuffer() {}

        using VulkanRawCommandBuffer::BeginRenderPass;
        using VulkanRawCommandBuffer::EndRenderPass;
        using VulkanRawCommandBuffer::SetVertexBuffers;
        using VulkanRawCommandBuffer::BindIndexBuffer;
        using VulkanRawCommandBuffer::SetScissor;
        using VulkanRawCommandBuffer::SetViewPort;

        VulkanRenderState renderState;
        Ref<VulkanFence> fence = nullptr;

        void SetRenderTarget(Window* window, uint32_t index) override final;
        void SetRenderTarget(Texture* renderTarget, uint32_t index) override final;
        void SetViewPort(uint4 rect, float mindepth, float maxdepth) override final;
        void SetScissor(uint4 rect) override final;

        void SetShader(const Shader* shader, uint variantIndex) override final;
        void SetVertexBuffers(const Buffer** buffers, uint count) override final;
        void SetIndexBuffer(const Buffer* buffer, size_t offset) override final;
        void SetBuffer(uint32_t nameHashId, const Buffer* buffer) override final;
        void SetTexture(uint32_t nameHashId, Texture* texture) override final;
        void SetConstant(uint32_t nameHashId, const void* data, uint32_t size) override final;

        inline void SetResolveTarget(const VulkanRenderTarget& renderTarget, uint32_t index) { renderState.SetResolveTarget(renderTarget, index); }
        inline void ClearColor(const color& color, uint32_t index) override final { renderState.ClearColor(color, index); }
        inline void ClearDepth(float depth, uint32_t stencil) override final { renderState.ClearDepth(depth, stencil); }
        inline void DiscardColor(uint32_t index) { renderState.DiscardColor(index); }
        inline void DiscardDepth() { renderState.DiscardDepth(); }

        inline void SetBlending(const BlendParameters& blend) override final { renderState.SetBlending(blend); }
        inline void SetRasterization(const RasterizationParameters& rasterization) override final { renderState.SetRasterization(rasterization); }
        inline void SetDepthStencil(const DepthStencilParameters& depthStencil) override final { renderState.SetDepthStencil(depthStencil); }
        inline void SetMultisampling(const MultisamplingParameters& multisampling) override final { renderState.SetMultisampling(multisampling); }

        void BeginRenderPass() override final;
        void EndRenderPass() override final;

        void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override final;
        void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) override final;
        
        void Blit(Texture* src, Window* dst, uint32_t dstLevel, uint32_t dstLayer, FilterMode filter) const override final;
        void Blit(Texture* src, Texture* dst, uint32_t srcLevel, uint32_t dstLevel, uint32_t srcLayer, uint32_t dstLayer, FilterMode filter) const override final;
        void Blit(const VulkanRenderTarget& src, const VulkanRenderTarget& dst, uint32_t srcLevel, uint32_t dstLevel, uint32_t srcLayer, uint32_t dstLayer, FilterMode filter) const;

        void ValidatePipeline();
    };
}
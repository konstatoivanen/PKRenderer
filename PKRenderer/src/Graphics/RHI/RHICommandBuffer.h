#pragma once
#include "Utilities/NoCopy.h"
#include "Utilities/NativeInterface.h"
#include "Math/Types.h"
#include "Graphics/RHI/RHI.h"

namespace PK::Graphics::RHI
{
    struct RHICommandBuffer : public PK::Utilities::NoCopy, public Utilities::NativeInterface<RHICommandBuffer>
    {
        virtual Utilities::FenceRef GetFenceRef() const = 0;
        virtual void SetRenderTarget(const Math::uint3& resolution) = 0;
        virtual void SetRenderTarget(RHITexture* const* renderTarget, RHITexture* const* resolveTargets, const TextureViewRange* ranges, uint32_t count) = 0;
        virtual void ClearColor(const Math::color& color, uint32_t index) = 0;
        virtual void ClearDepth(float depth, uint32_t stencil) = 0;
        virtual void DiscardColor(uint32_t index) = 0;
        virtual void DiscardDepth() = 0;

        virtual void SetViewPorts(const Math::uint4* rects, uint32_t count) = 0;
        virtual void SetScissors(const Math::uint4* rects, uint32_t count) = 0;
        virtual void SetUnorderedOverlap(bool value) = 0;

        virtual void SetStageExcludeMask(const ShaderStageFlags mask) = 0;
        virtual void SetBlending(const BlendParameters& blend) = 0;
        virtual void SetRasterization(const RasterizationParameters& rasterization) = 0;
        virtual void SetDepthStencil(const DepthStencilParameters& depthStencil) = 0;
        virtual void SetMultisampling(const MultisamplingParameters& multisampling) = 0;

        virtual void SetShader(const RHIShader* shader) = 0;
        virtual void SetVertexBuffers(const RHIBuffer** buffers, uint32_t count) = 0;
        virtual void SetVertexStreams(const VertexStreamElement* elements, uint32_t count) = 0;
        virtual void SetIndexBuffer(const RHIBuffer* buffer, size_t offset, ElementType indexType) = 0;
        virtual void SetShaderBindingTable(RayTracingShaderGroup group, const RHIBuffer* buffer, size_t offset = 0, size_t stride = 0, size_t size = 0) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
        virtual void DrawIndexedIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void DrawMeshTasks(const Math::uint3& dimensions) = 0;
        virtual void DrawMeshTasksIndirect(const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void DrawMeshTasksIndirectCount(const RHIBuffer* indirectArguments, size_t offset, const RHIBuffer* countBuffer, size_t countOffset, uint32_t maxDrawCount, uint32_t stride) = 0;
        virtual void Dispatch(const Math::uint3& dimensions) = 0;
        virtual void DispatchRays(const Math::uint3& dimensions) = 0;

        virtual void Blit(RHITexture* src, RHIWindow* dst, FilterMode filter) = 0;
        virtual void Blit(RHIWindow* src, RHIBuffer* dst) = 0;
        virtual void Blit(RHITexture* src, RHITexture* dst, const TextureViewRange& srcRange, const TextureViewRange& dstRange, FilterMode filter) = 0;

        virtual void Clear(RHIBuffer* dst, size_t offset, size_t size, uint32_t value) = 0;
        virtual void Clear(RHITexture* dst, const TextureViewRange& range, const Math::uint4& value) = 0;
        
        virtual void UpdateBuffer(RHIBuffer* dst, size_t offset, size_t size, void* data) = 0;
        virtual void* BeginBufferWrite(RHIBuffer* buffer, size_t offset, size_t size) = 0;
        virtual void EndBufferWrite(RHIBuffer* buffer) = 0;

        virtual void UploadTexture(RHITexture* texture, const void* data, size_t size, ImageUploadRange* ranges, uint32_t rangeCount) = 0;
        virtual void UploadTexture(RHITexture* texture, const void* data, size_t size, uint32_t level, uint32_t layer) = 0;

        virtual void BeginDebugScope(const char* name, const Math::color& color) = 0;
        virtual void EndDebugScope() = 0;
    };
}
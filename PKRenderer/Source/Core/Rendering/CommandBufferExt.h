#pragma once
#include "Core/Math/MathFwd.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    typedef std::initializer_list<const TextureViewRange> RenderTargetRanges;

    // Extended wrapper class with utility functions beyond the pure virtual interface
    struct CommandBufferExt
    {
        RHICommandBuffer* commandBuffer;
        
        CommandBufferExt() : commandBuffer(nullptr) {}
        CommandBufferExt(RHICommandBuffer* commandBuffer) : commandBuffer(commandBuffer) {}

        operator RHICommandBuffer* () const { return commandBuffer; }
        RHICommandBuffer* operator->() { return commandBuffer; }

        void SetViewPort(const uint4& rect);
        void SetScissor(const uint4& rect);

        void SetFixedStateAttributes(const FixedFunctionShaderAttributes* attribs);

        void SetShaderBindingTable(ShaderBindingTable* bindingTable);

        void SetShader(const ShaderAsset* shader, int32_t variantIndex = -1);

        void SetRenderTarget(const std::initializer_list<RHITexture*>& targets, const RenderTargetRanges& ranges, bool updateViewPort);
        void SetRenderTarget(const std::initializer_list<RHITexture*>& targets, bool updateViewPort);
        void SetRenderTarget(RHITexture* target);
        void SetRenderTarget(RHITexture* target, const TextureViewRange& range);
        void SetRenderTarget(RHITexture* target, uint16_t level, uint16_t layer);
        void SetRenderTarget(RHITexture* target, const RenderTargetRanges& ranges);

        void SetVertexStreams(const VertexStreamLayout& layout);

        void ResetBuiltInAtomicCounter();

        void Blit(const ShaderAsset* shader, int32_t variantIndex = -1);
        void Blit(const ShaderAsset* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex = -1);
        void Dispatch(const ShaderAsset* shader, uint3 dimensions);
        void Dispatch(const ShaderAsset* shader, uint32_t variantIndex, uint3 dimensions);
        void DispatchWithCounter(const ShaderAsset* shader, uint32_t variantIndex, uint3 dimensions);
        void DispatchWithCounter(const ShaderAsset* shader, uint3 dimensions);
        void DispatchRays(const ShaderAsset* shader, uint3 dimensions);
        void DispatchRays(const ShaderAsset* shader, uint32_t variantIndex, uint3 dimensions);

        void UploadBufferData(RHIBuffer* buffer, const void* data);
        void UploadBufferData(RHIBuffer* buffer, const void* data, size_t offset, size_t size);
        void UploadBufferSubData(RHIBuffer* buffer, const void* data, size_t offset, size_t size);

        template<typename T>
        BufferView<T> BeginBufferWrite(RHIBuffer* buffer)
        {
            size_t bufSize = buffer->GetSize();
            return { reinterpret_cast<T*>(commandBuffer->BeginBufferWrite(buffer, 0, bufSize)), bufSize / sizeof(T) };
        }

        template<typename T>
        BufferView<T> BeginBufferWrite(RHIBuffer* buffer, size_t offset, size_t count)
        {
            return { reinterpret_cast<T*>(commandBuffer->BeginBufferWrite(buffer, offset * sizeof(T), count * sizeof(T))), count };
        }

        template<typename T>
        InterleavedBufferView<T> BeginBufferWrite(RHIBuffer* buffer, size_t stride, size_t elementOffset, size_t bufferOffset, size_t count)
        {
            return { reinterpret_cast<uint8_t*>(commandBuffer->BeginBufferWrite(buffer, bufferOffset * stride, count * stride)), count, stride, elementOffset };
        }

        void SetMesh(const Mesh* mesh);
        void DrawMesh(const Mesh* mesh, int32_t submesh, uint32_t instanceCount, uint32_t firstInstance);
        void DrawMesh(const Mesh* mesh, int32_t submesh);
        void DrawMesh(const Mesh* mesh, int32_t submesh, const ShaderAsset* shader, int32_t variantIndex);
        void DrawMesh(const Mesh* mesh, int32_t submesh, const ShaderAsset* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex);
        void DrawMeshIndirect(const Mesh* mesh, const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride);
    };
}

#pragma once
#include "Math/Types.h"
#include "Utilities/BufferView.h"
#include "Graphics/GraphicsFwd.h"

namespace PK::Graphics
{
    typedef std::initializer_list<const RHI::TextureViewRange> RenderTargetRanges;

    // Extended wrapper class with utility functions beyond the pure virtual interface
    struct CommandBufferExt
    {
        RHI::RHICommandBuffer* commandBuffer;
        
        CommandBufferExt() : commandBuffer(nullptr) {}
        CommandBufferExt(RHI::RHICommandBuffer* commandBuffer) : commandBuffer(commandBuffer) {}

        operator RHI::RHICommandBuffer* () const { return commandBuffer; }
        RHI::RHICommandBuffer* operator->() { return commandBuffer; }

        void SetViewPort(const Math::uint4& rect);
        void SetScissor(const Math::uint4& rect);

        void SetFixedStateAttributes(const RHI::FixedFunctionShaderAttributes* attribs);

        void SetShaderBindingTable(ShaderBindingTable* bindingTable);

        void SetShader(const Shader* shader, int32_t variantIndex = -1);

        void SetRenderTarget(const std::initializer_list<Texture*>& targets, const RenderTargetRanges& ranges, bool updateViewPort);
        void SetRenderTarget(const std::initializer_list<Texture*>& targets, bool updateViewPort);
        void SetRenderTarget(Texture* target);
        void SetRenderTarget(Texture* target, const RHI::TextureViewRange& range);
        void SetRenderTarget(Texture* target, uint16_t level, uint16_t layer);
        void SetRenderTarget(Texture* target, const RenderTargetRanges& ranges);

        void SetVertexStreams(const RHI::VertexStreamLayout& layout);

        void ResetBuiltInAtomicCounter();

        void Blit(const Shader* shader, int32_t variantIndex = -1);
        void Blit(const Shader* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex = -1);
        void Dispatch(const Shader* shader, Math::uint3 dimensions);
        void Dispatch(const Shader* shader, uint32_t variantIndex, Math::uint3 dimensions);
        void DispatchWithCounter(const Shader* shader, uint32_t variantIndex, Math::uint3 dimensions);
        void DispatchWithCounter(const Shader* shader, Math::uint3 dimensions);
        void DispatchRays(const Shader* shader, Math::uint3 dimensions);
        void DispatchRays(const Shader* shader, uint32_t variantIndex, Math::uint3 dimensions);

        void UploadBufferData(Buffer* buffer, const void* data);
        void UploadBufferData(Buffer* buffer, const void* data, size_t offset, size_t size);
        void UploadBufferSubData(Buffer* buffer, const void* data, size_t offset, size_t size);

        // Hacky wrapper so that we donat have to expose buffer in this header.
        void* BeginBufferWrite(Buffer* buffer, size_t& outSize);

        template<typename T>
        Utilities::BufferView<T> BeginBufferWrite(Buffer* buffer)
        {
            size_t bufSize;
            auto pData = BeginBufferWrite(buffer, bufSize);
            return { reinterpret_cast<T*>(pData), bufSize / sizeof(T) };
        }

        template<typename T>
        Utilities::BufferView<T> BeginBufferWrite(Buffer* buffer, size_t offset, size_t count)
        {
            return { reinterpret_cast<T*>(commandBuffer->BeginBufferWrite(buffer, offset * sizeof(T), count * sizeof(T))), count };
        }

        template<typename T>
        Utilities::InterleavedBufferView<T> BeginBufferWrite(Buffer* buffer, size_t stride, size_t elementOffset, size_t bufferOffset, size_t count)
        {
            return { reinterpret_cast<uint8_t*>(commandBuffer->BeginBufferWrite(buffer, bufferOffset * stride, count * stride)), count, stride, elementOffset };
        }

        void SetMesh(const Mesh* mesh);
        void DrawMesh(const Mesh* mesh, int32_t submesh, uint32_t instanceCount, uint32_t firstInstance);
        void DrawMesh(const Mesh* mesh, int32_t submesh);
        void DrawMesh(const Mesh* mesh, int32_t submesh, const Shader* shader, int32_t variantIndex);
        void DrawMesh(const Mesh* mesh, int32_t submesh, const Shader* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex);
        void DrawMeshIndirect(const Mesh* mesh, const Buffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride);
    };
}

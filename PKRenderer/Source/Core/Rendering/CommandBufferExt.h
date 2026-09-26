#pragma once
#include "Core/Base/Containers/InitializerList.h"
#include "Core/Math/Forward.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    template<typename T>
    struct StagedBufferView
    {
        RHIBuffer* stage = nullptr;
        size_t offset;
        size_t size;

        T* data = nullptr;
        size_t count = 0;
        T& operator[](size_t index) { return data[index]; }
    };

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

        void SetRenderTarget(const initializer_list<RenderTargetBinding>& targets, bool updateViewPort);
        void SetRenderTarget(const RenderTargetBinding& binding, bool updateViewPort = false);
        void SetRenderTarget(const uint2& resolution, uint32_t layerCount);

        void SetVertexStreams(const VertexStreamLayout& layout);

        void Blit(const ShaderAsset* shader, int32_t variantIndex = -1);
        void Blit(const ShaderAsset* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex = -1);
        void Dispatch(const ShaderAsset* shader, uint3 dimensions);
        void Dispatch(const ShaderAsset* shader, uint32_t variantIndex, uint3 dimensions);
        void DispatchRays(const ShaderAsset* shader, uint3 dimensions);
        void DispatchRays(const ShaderAsset* shader, uint32_t variantIndex, uint3 dimensions);

        void UploadBufferData(RHIBuffer* buffer, const void* data, size_t offset = 0ull, size_t size = 0ull);

        template<typename T>
        StagedBufferView<T> BeginBufferWrite([[maybe_unused]] RHIBuffer* buffer, size_t first = 0ull, size_t count = 0ull)
        {
            auto offset = first * sizeof(T);
            auto size = count ? count * sizeof(T) : (buffer->GetSize() / sizeof(T));
            auto stage = commandBuffer->AcquireStagingBuffer(size);
            return { stage, offset, size, reinterpret_cast<T*>(stage->BeginMap(0ull,0ull)), count };
        }

        template<typename T>
        void EndBufferWrite(RHIBuffer* buffer, const StagedBufferView<T>& view)
        {
            commandBuffer->CopyBuffer(buffer, view.stage, 0ull, view.offset, view.size);
            commandBuffer->ReleaseStagingBuffer(view.stage);
        }

        void UploadTexture(RHITexture* texture, const void* data, size_t size, TextureDataRegion* regions, uint32_t regionCount);
        void UploadTexture(RHITexture* texture, const void* data, size_t size, uint32_t level, uint32_t layer, uint32_t layers);

        void SetMesh(const IMesh* mesh);
        void DrawMesh(const IMesh* mesh, int32_t submesh, uint32_t instanceCount, uint32_t firstInstance);
        void DrawMesh(const IMesh* mesh, int32_t submesh);
        void DrawMesh(const IMesh* mesh, int32_t submesh, const ShaderAsset* shader, int32_t variantIndex);
        void DrawMesh(const IMesh* mesh, int32_t submesh, const ShaderAsset* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex);
        void DrawMeshIndirect(const IMesh* mesh, const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride);

        void BeginStatScope(NameID name, const float4& color);
        void EndStatScope();
    };
}

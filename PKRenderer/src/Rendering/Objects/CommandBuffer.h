#pragma once
#include "Utilities/NoCopy.h"
#include "Core/Window.h"
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/BindArray.h"

namespace PK::Rendering::Objects
{
    typedef struct RenderTargets { Texture* targets[PK_MAX_RENDER_TARGETS + 1]; } RenderTargets;
    typedef std::initializer_list<const TextureViewRange> RenderTargetRanges;

    struct CommandBuffer : public PK::Utilities::NoCopy
    {
        virtual void SetRenderTarget(Texture** renderTarget, Texture** resolveTargets, const TextureViewRange* ranges, uint32_t count) = 0;
        virtual void ClearColor(const color& color, uint32_t index) = 0;
        virtual void ClearDepth(float depth, uint32_t stencil) = 0;
        virtual void DiscardColor(uint32_t index) = 0;
        virtual void DiscardDepth() = 0;

        virtual void SetViewPort(uint4 rect, float mindepth, float maxdepth, uint index = 0) = 0;
        virtual void SetScissor(uint4 rect, uint index = 0) = 0;

        virtual void SetBlending(const BlendParameters& blend) = 0;
        virtual void SetRasterization(const RasterizationParameters& rasterization) = 0;
        virtual void SetDepthStencil(const DepthStencilParameters& depthStencil) = 0;
        virtual void SetMultisampling(const MultisamplingParameters& multisampling) = 0;

        virtual void SetShader(const Shader* shader, int variantIndex = -1) = 0;
        virtual void SetVertexBuffers(const Buffer** buffers, uint count) = 0;
        virtual void SetIndexBuffer(const Buffer* buffer, size_t offset) = 0;
        virtual void SetBuffer(uint32_t nameHashId, Buffer* buffer, const IndexRange& range) = 0;
        virtual void SetTexture(uint32_t nameHashId, Texture* texture, const TextureViewRange& range) = 0;
        virtual void SetBufferArray(uint32_t nameHashId, BindArray<Buffer>* bufferArray) = 0;
        virtual void SetTextureArray(uint32_t nameHashId, BindArray<Texture>* textureArray) = 0;
        virtual void SetImage(uint32_t nameHashId, Texture* texture, const TextureViewRange& range) = 0;
        virtual void SetConstant(uint32_t nameHashId, const void* data, uint32_t size) = 0;
        virtual void SetKeyword(uint32_t nameHashId, bool value) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
        virtual void Dispatch(uint3 groupCount) = 0;
        
        // @TODO Nasty dependency. Rethink this one!
        virtual void Blit(Texture* src, Window* dst, uint32_t dstLevel, uint32_t dstLayer, FilterMode filter) = 0;
        virtual void Blit(Texture* src, Texture* dst, uint32_t srcLevel, uint32_t dstLevel, uint32_t srcLayer, uint32_t dstLayer, FilterMode filter) = 0;
        
        virtual void Barrier(const Texture* texture, const TextureViewRange& range, const Buffer* buffer, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags) = 0;

        void SetRenderTarget(RenderTexture* renderTarget, bool updateViewPort = true);
        void SetRenderTarget(Texture* renderTarget);
        void SetRenderTarget(Texture* renderTarget, const TextureViewRange& range);
        void SetRenderTarget(Texture* renderTarget, ushort level, ushort layer);
        void SetRenderTarget(Texture* renderTarget, const RenderTargetRanges& ranges);
        void SetRenderTarget(RenderTargets& renderTargets, const RenderTargetRanges& ranges);
        void SetRenderTarget(RenderTargets& renderTargets, RenderTargets& resolveTargets, const RenderTargetRanges& ranges);
        void SetMesh(const Mesh* mesh);
        void SetBuffer(uint32_t nameHashId, Buffer* buffer);
        void SetBuffer(const char* name, Buffer* buffer);
        void SetBuffer(const char* name, Buffer* buffer, const IndexRange& range);

        void SetTexture(uint32_t nameHashId, Texture* texture);
        void SetTexture(uint32_t nameHashId, Texture* texture, ushort level, ushort layer);
        void SetTexture(const char* name, Texture* texture);
        void SetTexture(const char* name, Texture* texture, ushort level, ushort layer);
        void SetTexture(const char* name, Texture* texture, const TextureViewRange& range);
        
        void SetImage(uint32_t nameHashId, Texture* texture);
        void SetImage(uint32_t nameHashId, Texture* texture, ushort level, ushort layer);
        void SetImage(const char* name, Texture* texture);
        void SetImage(const char* name, Texture* texture, ushort level, ushort layer);
        void SetImage(const char* name, Texture* texture, const TextureViewRange& range);
        
        void SetBufferArray(const char* name, BindArray<Buffer>* bufferArray);
        void SetTextureArray(const char* name, BindArray<Texture>* textureArray);

        void SetConstant(const char* name, const void* data, uint32_t size);
        void SetKeyword(const char* name, bool value);

        template<typename T>
        void SetConstant(uint32_t nameHashId, const T& value) { SetConstant(nameHashId, &value, (uint32_t)sizeof(T)); }

        template<typename T>
        void SetConstant(const char* name, const T& value) { SetConstant(name, &value, (uint32_t)sizeof(T)); }
        
        void DrawMesh(const Mesh* mesh, int submesh);
        void DrawMesh(const Mesh* mesh, int submesh, uint32_t instanceCount, uint32_t firstInstance);
        void DrawMesh(const Mesh* mesh, int submesh, Shader* shader, int variantIndex = -1);
        void DrawMesh(const Mesh* mesh, int submesh, Shader* shader, uint32_t instanceCount, uint32_t firstInstance, int variantIndex = -1);
        void Blit(Shader* shader, int variantIndex = -1);
        void Dispatch(Shader* shader, uint3 groupCount);
        void Dispatch(Shader* shader, uint variantIndex, uint3 groupCount);
        
        void Barrier(const Texture* texture, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags);
        void Barrier(const Texture* texture, const TextureViewRange& range, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags);
        void Barrier(const Texture* texture, ushort level, ushort layer, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags);
        void Barrier(const Buffer* buffer, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags);
        void Barrier(MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags);
    };
}
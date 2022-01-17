#pragma once
#include "Utilities/NoCopy.h"
#include "Core/Window.h"
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/BindArray.h"
#include "Rendering/Structs/ExecutionGate.h"

namespace PK::Rendering::Objects
{
    typedef std::initializer_list<const TextureViewRange> RenderTargetRanges;

    struct CommandBuffer : public PK::Utilities::NoCopy
    {
        virtual ExecutionGate GetOnCompleteGate() const = 0;
        virtual void SetRenderTarget(const uint3& resolution) = 0;
        virtual void SetRenderTarget(Texture** renderTarget, Texture** resolveTargets, const TextureViewRange* ranges, uint32_t count) = 0;
        virtual void ClearColor(const color& color, uint32_t index) = 0;
        virtual void ClearDepth(float depth, uint32_t stencil) = 0;
        virtual void DiscardColor(uint32_t index) = 0;
        virtual void DiscardDepth() = 0;

        virtual void SetViewPort(uint4 rect, uint index = 0) = 0;
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
        virtual void DrawIndexedIndirect(const Buffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void Dispatch(uint3 groupCount) = 0;
        
        // @TODO Nasty dependency. Rethink this one!
        virtual void Blit(Texture* src, Window* dst, uint32_t dstLevel, uint32_t dstLayer, FilterMode filter) = 0;
        virtual void Blit(Texture* src, Texture* dst, uint32_t srcLevel, uint32_t dstLevel, uint32_t srcLayer, uint32_t dstLayer, FilterMode filter) = 0;

        virtual void Clear(Buffer* dst, size_t offset, size_t size, uint32_t value) = 0;
        virtual void Clear(Texture* dst, const TextureViewRange& range, const uint4& value) = 0;
        
        virtual void Barrier(const Texture* texture, const TextureViewRange& range, const Buffer* buffer, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags) = 0;

        void SetFixedStateAttributes(FixedFunctionShaderAttributes* attribs);

        void SetRenderTarget(RenderTexture* renderTarget, const uint32_t* targets, uint32_t targetCount, bool bindDepth, bool updateViewPort);
        void SetRenderTarget(RenderTexture* renderTarget, std::initializer_list<uint32_t> targets, bool bindDepth, bool updateViewPort);
        void SetRenderTarget(RenderTexture* renderTarget, bool updateViewPort);
        void SetRenderTarget(Texture* renderTarget);
        void SetRenderTarget(Texture* renderTarget, const TextureViewRange& range);
        void SetRenderTarget(Texture* renderTarget, ushort level, ushort layer);
        void SetRenderTarget(Texture* renderTarget, const RenderTargetRanges& ranges);
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
        void DrawMesh(const Mesh* mesh, int submesh, const Shader* shader, int variantIndex = -1);
        void DrawMesh(const Mesh* mesh, int submesh, const Shader* shader, uint32_t instanceCount, uint32_t firstInstance, int variantIndex = -1);
        void Blit(const Shader* shader, int variantIndex = -1);
        void Blit(const Shader* shader, uint32_t instanceCount, uint32_t firstInstance, int variantIndex = -1);
        void Dispatch(const Shader* shader, uint3 groupCount);
        void Dispatch(const Shader* shader, uint variantIndex, uint3 groupCount);
        
        void Barrier(const Texture* texture, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags);
        void Barrier(const Texture* texture, const TextureViewRange& range, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags);
        void Barrier(const Texture* texture, ushort level, ushort layer, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags);
        void Barrier(const Buffer* buffer, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags);
        void Barrier(MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags);
    };
}
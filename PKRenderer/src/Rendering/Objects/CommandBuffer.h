#pragma once
#include "Utilities/NoCopy.h"
#include "Core/Window.h"
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/RenderTexture.h"
#include "Rendering/Objects/AccelerationStructure.h"
#include "Rendering/Objects/BindArray.h"
#include "Rendering/Structs/ExecutionGate.h"

namespace PK::Rendering::Objects
{
    typedef std::initializer_list<const Structs::TextureViewRange> RenderTargetRanges;

    // @TODO refactor object command buffer operations to happen through command buffers as under the hood they're dependent anyway.
    // Current setup hides implicit dependencies on currently active command buffers.
    struct CommandBuffer : public PK::Utilities::NoCopy, public Utilities::NativeInterface<CommandBuffer>
    {
        virtual Structs::ExecutionGate GetOnCompleteGate() const = 0;
        virtual void SetRenderTarget(const Math::uint3& resolution) = 0;
        virtual void SetRenderTarget(Texture** renderTarget, Texture** resolveTargets, const Structs::TextureViewRange* ranges, uint32_t count) = 0;
        virtual void ClearColor(const Math::color& color, uint32_t index) = 0;
        virtual void ClearDepth(float depth, uint32_t stencil) = 0;
        virtual void DiscardColor(uint32_t index) = 0;
        virtual void DiscardDepth() = 0;

        virtual void SetViewPorts(const Math::uint4* rects, uint32_t count) = 0;
        virtual void SetScissors(const Math::uint4* rects, uint32_t count) = 0;

        virtual void SetBlending(const Structs::BlendParameters& blend) = 0;
        virtual void SetRasterization(const Structs::RasterizationParameters& rasterization) = 0;
        virtual void SetDepthStencil(const Structs::DepthStencilParameters& depthStencil) = 0;
        virtual void SetMultisampling(const Structs::MultisamplingParameters& multisampling) = 0;

        virtual void SetShader(const Shader* shader, int32_t variantIndex = -1) = 0;
        virtual void SetVertexBuffers(const Buffer** buffers, uint32_t count) = 0;
        virtual void SetIndexBuffer(const Buffer* buffer, size_t offset) = 0;
        virtual void SetBuffer(uint32_t nameHashId, Buffer* buffer, const Structs::IndexRange& range) = 0;
        virtual void SetTexture(uint32_t nameHashId, Texture* texture, const Structs::TextureViewRange& range) = 0;
        virtual void SetBufferArray(uint32_t nameHashId, BindArray<Buffer>* bufferArray) = 0;
        virtual void SetTextureArray(uint32_t nameHashId, BindArray<Texture>* textureArray) = 0;
        virtual void SetImage(uint32_t nameHashId, Texture* texture, const Structs::TextureViewRange& range) = 0;
        virtual void SetAccelerationStructure(uint32_t nameHashId, AccelerationStructure* structure) = 0;
        virtual void SetShaderBindingTable(Structs::RayTracingShaderGroup group, const Buffer* buffer, size_t offset = 0, size_t stride = 0, size_t size = 0) = 0;
        virtual void SetConstant(uint32_t nameHashId, const void* data, uint32_t size) = 0;
        virtual void SetKeyword(uint32_t nameHashId, bool value) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
        virtual void DrawIndexedIndirect(const Buffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void Dispatch(Math::uint3 groupCount) = 0;
        virtual void DispatchRays(Math::uint3 dimensions) = 0;

        // @TODO Nasty dependency. Rethink this one!
        virtual void Blit(Texture* src, Core::Window* dst, Structs::FilterMode filter) = 0;
        virtual void Blit(Core::Window* src, Buffer* dst) = 0;
        virtual void Blit(Texture* src, Texture* dst, const Structs::TextureViewRange& srcRange, const Structs::TextureViewRange& dstRange, Structs::FilterMode filter) = 0;

        virtual void Clear(Buffer* dst, size_t offset, size_t size, uint32_t value) = 0;
        virtual void Clear(Texture* dst, const Structs::TextureViewRange& range, const Math::uint4& value) = 0;
        
        virtual void Barrier(const Texture* texture, const Structs::TextureViewRange& range, const Buffer* buffer, size_t offset, size_t size, Structs::MemoryAccessFlags srcFlags, Structs::MemoryAccessFlags dstFlags) = 0;

        virtual void BeginDebugScope(const char* name, const Math::color& color) = 0;
        virtual void EndDebugScope() = 0;

        void SetViewPort(const Math::uint4& rect);
        void SetScissor(const Math::uint4& rect);

        void SetFixedStateAttributes(Structs::FixedFunctionShaderAttributes* attribs);

        void SetRenderTarget(RenderTexture* renderTarget, const uint32_t* targets, uint32_t targetCount, bool bindDepth, bool updateViewPort);
        void SetRenderTarget(RenderTexture* renderTarget, std::initializer_list<uint32_t> targets, bool bindDepth, bool updateViewPort);
        void SetRenderTarget(RenderTexture* renderTarget, bool updateViewPort);
        void SetRenderTarget(Texture* renderTarget);
        void SetRenderTarget(Texture* renderTarget, const Structs::TextureViewRange& range);
        void SetRenderTarget(Texture* renderTarget, uint16_t level, uint16_t layer);
        void SetRenderTarget(Texture* renderTarget, const RenderTargetRanges& ranges);
        void SetMesh(const Mesh* mesh);
        void SetBuffer(uint32_t nameHashId, Buffer* buffer);
        void SetBuffer(const char* name, Buffer* buffer);
        void SetBuffer(const char* name, Buffer* buffer, const Structs::IndexRange& range);

        void SetTexture(uint32_t nameHashId, Texture* texture);
        void SetTexture(uint32_t nameHashId, Texture* texture, uint16_t level, uint16_t layer);
        void SetTexture(const char* name, Texture* texture);
        void SetTexture(const char* name, Texture* texture, uint16_t level, uint16_t layer);
        void SetTexture(const char* name, Texture* texture, const Structs::TextureViewRange& range);
        
        void SetImage(uint32_t nameHashId, Texture* texture);
        void SetImage(uint32_t nameHashId, Texture* texture, uint16_t level, uint16_t layer);
        void SetImage(const char* name, Texture* texture);
        void SetImage(const char* name, Texture* texture, uint16_t level, uint16_t layer);
        void SetImage(const char* name, Texture* texture, const Structs::TextureViewRange& range);
        
        void SetAccelerationStructure(const char* name, AccelerationStructure* structure);

        void SetBufferArray(const char* name, BindArray<Buffer>* bufferArray);
        void SetTextureArray(const char* name, BindArray<Texture>* textureArray);

        void SetConstant(const char* name, const void* data, uint32_t size);
        void SetKeyword(const char* name, bool value);

        template<typename T>
        void SetConstant(uint32_t nameHashId, const T& value) { SetConstant(nameHashId, &value, (uint32_t)sizeof(T)); }

        template<typename T>
        void SetConstant(const char* name, const T& value) { SetConstant(name, &value, (uint32_t)sizeof(T)); }
        
        void DrawMesh(const Mesh* mesh, int32_t submesh);
        void DrawMesh(const Mesh* mesh, int32_t submesh, uint32_t instanceCount, uint32_t firstInstance);
        void DrawMesh(const Mesh* mesh, int32_t submesh, const Shader* shader, int32_t variantIndex = -1);
        void DrawMesh(const Mesh* mesh, int32_t submesh, const Shader* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex = -1);
        void DrawMeshIndirect(const Mesh* mesh, const Buffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride);
        void Blit(const Shader* shader, int32_t variantIndex = -1);
        void Blit(const Shader* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex = -1);
        void Dispatch(const Shader* shader, Math::uint3 groupCount);
        void Dispatch(const Shader* shader, uint32_t variantIndex, Math::uint3 groupCount);
        void DispatchRays(const Shader* shader, Math::uint3 dimensions);
        void DispatchRays(const Shader* shader, uint32_t variantIndex, Math::uint3 dimensions);
        
        void Barrier(const Texture* texture, Structs::MemoryAccessFlags srcFlags, Structs::MemoryAccessFlags dstFlags);
        void Barrier(const Texture* texture, const Structs::TextureViewRange& range, Structs::MemoryAccessFlags srcFlags, Structs::MemoryAccessFlags dstFlags);
        void Barrier(const Texture* texture, uint16_t level, uint16_t layer, Structs::MemoryAccessFlags srcFlags, Structs::MemoryAccessFlags dstFlags);
        void Barrier(const Buffer* buffer, Structs::MemoryAccessFlags srcFlags, Structs::MemoryAccessFlags dstFlags);
        void Barrier(const Buffer* buffer, size_t offset, size_t size, Structs::MemoryAccessFlags srcFlags, Structs::MemoryAccessFlags dstFlags);
        void Barrier(Structs::MemoryAccessFlags srcFlags, Structs::MemoryAccessFlags dstFlags);
    };
}
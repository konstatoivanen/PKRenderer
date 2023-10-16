#pragma once
#include "Utilities/NoCopy.h"
#include "Core/Window.h"
#include "Rendering/Structs/ImageUploadRange.h"
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/AccelerationStructure.h"
#include "Rendering/Objects/BindArray.h"
#include "Rendering/Structs/FenceRef.h"

namespace PK::Rendering::Objects
{
    typedef std::initializer_list<const Structs::TextureViewRange> RenderTargetRanges;

    struct CommandBuffer : public PK::Utilities::NoCopy, public Utilities::NativeInterface<CommandBuffer>
    {
        virtual Structs::FenceRef GetFenceRef() const = 0;
        virtual void SetRenderTarget(const Math::uint3& resolution) = 0;
        virtual void SetRenderTarget(Texture* const* renderTarget, Texture* const* resolveTargets, const Structs::TextureViewRange* ranges, uint32_t count) = 0;
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
        virtual void SetShaderBindingTable(Structs::RayTracingShaderGroup group, const Buffer* buffer, size_t offset = 0, size_t stride = 0, size_t size = 0) = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
        virtual void DrawIndexedIndirect(const Buffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride) = 0;
        virtual void Dispatch(Math::uint3 dimensions) = 0;
        virtual void DispatchRays(Math::uint3 dimensions) = 0;

        // @TODO Nasty dependency. Rethink this one!
        virtual void Blit(Texture* src, Core::Window* dst, Structs::FilterMode filter) = 0;
        virtual void Blit(Core::Window* src, Buffer* dst) = 0;
        virtual void Blit(Texture* src, Texture* dst, const Structs::TextureViewRange& srcRange, const Structs::TextureViewRange& dstRange, Structs::FilterMode filter) = 0;

        virtual void Clear(Buffer* dst, size_t offset, size_t size, uint32_t value) = 0;
        virtual void Clear(Texture* dst, const Structs::TextureViewRange& range, const Math::uint4& value) = 0;
        
        virtual void* BeginBufferWrite(Buffer* buffer, size_t offset, size_t size) = 0;
        virtual void EndBufferWrite(Buffer* buffer) = 0;

        virtual void UploadTexture(Texture* texture, const void* data, size_t size, Structs::ImageUploadRange* ranges, uint32_t rangeCount) = 0;
        virtual void UploadTexture(Texture* texture, const void* data, size_t size, uint32_t level, uint32_t layer) = 0;

        virtual void BeginDebugScope(const char* name, const Math::color& color) = 0;
        virtual void EndDebugScope() = 0;

        void SetViewPort(const Math::uint4& rect);
        void SetScissor(const Math::uint4& rect);

        void SetFixedStateAttributes(Structs::FixedFunctionShaderAttributes* attribs);

        void SetRenderTarget(const std::initializer_list<Texture*>& targets, const RenderTargetRanges& ranges, bool updateViewPort);
        void SetRenderTarget(const std::initializer_list<Texture*>& targets, bool updateViewPort);
        void SetRenderTarget(Texture* target);
        void SetRenderTarget(Texture* target, const Structs::TextureViewRange& range);
        void SetRenderTarget(Texture* target, uint16_t level, uint16_t layer);
        void SetRenderTarget(Texture* target, const RenderTargetRanges& ranges);
        void SetMesh(const Mesh* mesh);
        
        void DrawMesh(const Mesh* mesh, int32_t submesh);
        void DrawMesh(const Mesh* mesh, int32_t submesh, uint32_t instanceCount, uint32_t firstInstance);
        void DrawMesh(const Mesh* mesh, int32_t submesh, const Shader* shader, int32_t variantIndex = -1);
        void DrawMesh(const Mesh* mesh, int32_t submesh, const Shader* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex = -1);
        void DrawMeshIndirect(const Mesh* mesh, const Buffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride);
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

        template<typename T>
        Utilities::BufferView<T> BeginBufferWrite(Buffer* buffer)
        {
            auto bufSize = buffer->GetCapacity();
            return { reinterpret_cast<T*>(BeginBufferWrite(buffer, 0, bufSize)), bufSize / sizeof(T) };
        }

        template<typename T>
        Utilities::BufferView<T> BeginBufferWrite(Buffer* buffer, size_t offset, size_t count)
        {
            auto tsize = sizeof(T);
            auto mapSize = tsize * count + tsize * offset;
            auto bufSize = buffer->GetCapacity();

            PK_THROW_ASSERT(mapSize <= bufSize, "Map buffer range exceeds buffer bounds, map size: %i, buffer size: %i", mapSize, bufSize);

            return { reinterpret_cast<T*>(BeginBufferWrite(buffer, offset * tsize, count * tsize)), count };
        }

        template<typename T>
        Utilities::InterleavedBufferView<T> BeginBufferWrite(Buffer* buffer, size_t stride, size_t elementOffset, size_t bufferOffset, size_t count)
        {
            auto mapSize = stride * count + stride * bufferOffset;
            auto bufSize = buffer->GetCapacity();

            PK_THROW_ASSERT(mapSize <= bufSize, "Map buffer range exceeds buffer bounds, map size: %i, buffer size: %i", mapSize, bufSize);

            return { reinterpret_cast<uint8_t*>(BeginBufferWrite(buffer, bufferOffset * stride, count * stride)), count, stride, elementOffset };
        }
    };
}
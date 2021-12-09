#pragma once
#include "Core/NoCopy.h"
#include "Core/Window.h"
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/RenderTexture.h"

namespace PK::Rendering::Objects
{
    struct CommandBuffer : public PK::Core::NoCopy
    {
        // @TODO Nasty dependency. Rethink this one!
        virtual void SetRenderTarget(Window* window, uint32_t index) = 0;
        virtual void SetRenderTarget(Texture* renderTarget, uint32_t index) = 0;
        virtual void ClearColor(const color& color, uint32_t index) = 0;
        virtual void ClearDepth(float depth, uint32_t stencil) = 0;

        virtual void SetViewPort(uint4 rect, float mindepth, float maxdepth) = 0;
        virtual void SetScissor(uint4 rect) = 0;

        virtual void SetBlending(const BlendParameters& blend) = 0;
        virtual void SetRasterization(const RasterizationParameters& rasterization) = 0;
        virtual void SetDepthStencil(const DepthStencilParameters& depthStencil) = 0;
        virtual void SetMultisampling(const MultisamplingParameters& multisampling) = 0;

        virtual void SetShader(const Shader* shader, uint variantIndex) = 0;
        virtual void SetVertexBuffers(const Buffer** buffers, uint count) = 0;
        virtual void SetIndexBuffer(const Buffer* buffer, size_t offset) = 0;
        virtual void SetBuffer(uint32_t nameHashId, const Buffer* buffer) = 0;
        virtual void SetTexture(uint32_t nameHashId, Texture* texture) = 0;
        virtual void SetConstant(uint32_t nameHashId, const void* data, uint32_t size) = 0;

        virtual void BeginRenderPass() = 0;
        virtual void EndRenderPass() = 0;

        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) = 0;
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;
        virtual void DispatchCompute(const ShaderVariant* variant, uint3 groupCount) = 0;
        
        // @TODO Nasty dependency. Rethink this one!
        virtual void Blit(Texture* src, Window* dst, uint32_t dstLevel, uint32_t dstLayer, FilterMode filter) const = 0;
        virtual void Blit(Texture* src, Texture* dst, uint32_t srcLevel, uint32_t dstLevel, uint32_t srcLayer, uint32_t dstLayer, FilterMode filter) const = 0;
        
        virtual void Barrier(const Texture* texture, const Buffer* buffer, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags) const = 0;

        void SetRenderTarget(RenderTexture* renderTarget);
        void SetMesh(const Mesh* mesh);
        void SetBuffer(const char* name, const Buffer* buffer);
        void SetTexture(const char* name, Texture* texture);
        void SetConstant(const char* name, const void* data, uint32_t size);

        template<typename T>
        void SetConstant(uint32_t nameHashId, const T& value) { SetConstant(nameHashId, &value, (uint32_t)sizeof(T)); }

        template<typename T>
        void SetConstant(const char* name, const T& value) { SetConstant(name, &value, (uint32_t)sizeof(T)); }
        
        void DrawMesh(const Mesh* mesh, int submesh);
        void DispatchCompute(Shader* shader, uint variantIndex, uint3 groupCount);
        
        void Barrier(const Texture* texture, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags) const;
        void Barrier(const Buffer* buffer, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags) const;
        void Barrier(MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags) const;
    };
}
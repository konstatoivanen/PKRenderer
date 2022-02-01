#include "PrecompiledHeader.h"
#include "CommandBuffer.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Math;
    using namespace PK::Core::Services;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::Structs;

    void CommandBuffer::SetViewPort(const uint4& rect)
    {
        SetViewPorts(&rect, 1);
    }

    void CommandBuffer::SetScissor(const uint4& rect)
    {
        SetScissors(&rect, 1);
    }

    void CommandBuffer::SetFixedStateAttributes(FixedFunctionShaderAttributes* attribs)
    {
        if (attribs == nullptr)
        {
            return;
        }

        SetBlending(attribs->blending);
        SetDepthStencil(attribs->depthStencil);
        SetRasterization(attribs->rasterization);
    }

    void CommandBuffer::SetRenderTarget(RenderTexture* renderTarget, const uint32_t* targets, uint32_t targetCount, bool bindDepth, bool updateViewPort)
    {
        auto i = 0u;
        Texture* renderTargets[PK_MAX_RENDER_TARGETS + 1]{};
        TextureViewRange ranges[PK_MAX_RENDER_TARGETS + 1]{};

        for (; i < targetCount; ++i)
        {
            renderTargets[i] = renderTarget->GetColor(targets[i]);
        }

        auto depth = renderTarget->GetDepth();

        if (bindDepth && depth != nullptr)
        {
            renderTargets[i++] = depth;
        }

        SetRenderTarget(renderTargets, nullptr, ranges, i);

        if (updateViewPort)
        {
            auto rect = renderTarget->GetRect();
            SetViewPort(rect);
            SetScissor(rect);
        }
    }

    void CommandBuffer::SetRenderTarget(RenderTexture* renderTarget, std::initializer_list<uint32_t> targets, bool bindDepth, bool updateViewPort)
    {
        uint32_t count = (uint32_t)(targets.end() - targets.begin());
        SetRenderTarget(renderTarget, targets.begin(), count, bindDepth, updateViewPort);
    }

    void CommandBuffer::SetRenderTarget(RenderTexture* renderTarget, bool updateViewPort)
    {
        auto count = renderTarget->GetColorCount();
        uint32_t indices[PK_MAX_RENDER_TARGETS];
    
        for (auto i = 0u; i < count; ++i)
        {
            indices[i] = i;
        }

        SetRenderTarget(renderTarget, indices, count, true, updateViewPort);
    }

    void CommandBuffer::SetRenderTarget(Texture* renderTarget)
    {
        TextureViewRange range{};
        SetRenderTarget(&renderTarget, nullptr, &range, 1u);
    }

    void CommandBuffer::SetRenderTarget(Texture* renderTarget, const TextureViewRange& range)
    {
        SetRenderTarget(&renderTarget, nullptr, &range, 1u);
    }

    void CommandBuffer::SetRenderTarget(Texture* renderTarget, uint16_t level, uint16_t layer)
    {
        TextureViewRange range = { level, layer, 1u, 1u };
        SetRenderTarget(&renderTarget, nullptr, &range, 1u);
    }

    void CommandBuffer::SetRenderTarget(Texture* renderTarget, const RenderTargetRanges& ranges)
    {
        auto count = (uint32_t)(ranges.end() - ranges.begin());
        auto targets = PK_STACK_ALLOC(Texture*, count);

        for (auto i = 0u; i < count; ++i)
        {
            targets[i] = renderTarget;
        }

        SetRenderTarget(targets, nullptr, ranges.begin(), count);
    }

    void CommandBuffer::SetBuffer(const char* name, Buffer* buffer, const IndexRange& range) { SetBuffer(StringHashID::StringToID(name), buffer, range); }
    void CommandBuffer::SetBuffer(uint32_t nameHashId, Buffer* buffer) { SetBuffer(nameHashId, buffer, buffer->GetFullRange()); }
    void CommandBuffer::SetBuffer(const char* name, Buffer* buffer) { SetBuffer(StringHashID::StringToID(name), buffer, buffer->GetFullRange()); }

    void CommandBuffer::SetTexture(uint32_t nameHashId, Texture* texture) { SetTexture(nameHashId, texture, {}); }
    void CommandBuffer::SetTexture(uint32_t nameHashId, Texture* texture, uint16_t level, uint16_t layer) { SetTexture(nameHashId, texture, { level, layer, 1u, 1u }); }
    void CommandBuffer::SetTexture(const char* name, Texture* texture) { SetTexture(StringHashID::StringToID(name), texture); }
    void CommandBuffer::SetTexture(const char* name, Texture* texture, uint16_t level, uint16_t layer) { SetTexture(StringHashID::StringToID(name), texture, level, layer); }
    void CommandBuffer::SetTexture(const char* name, Texture* texture, const TextureViewRange& range) { SetTexture(StringHashID::StringToID(name), texture, range); }
    
    void CommandBuffer::SetImage(uint32_t nameHashId, Texture* texture) { SetImage(nameHashId, texture, {}); }
    void CommandBuffer::SetImage(uint32_t nameHashId, Texture* texture, uint16_t level, uint16_t layer) { SetImage(nameHashId, texture, { level, layer, 1u, 1u }); }
    void CommandBuffer::SetImage(const char* name, Texture* texture) { SetImage(StringHashID::StringToID(name), texture); }
    void CommandBuffer::SetImage(const char* name, Texture* texture, uint16_t level, uint16_t layer) { SetImage(StringHashID::StringToID(name), texture, level, layer); }
    void CommandBuffer::SetImage(const char* name, Texture* texture, const TextureViewRange& range) { SetImage(StringHashID::StringToID(name), texture, range); }

    void CommandBuffer::SetBufferArray(const char* name, BindArray<Buffer>* bufferArray) { SetBufferArray(StringHashID::StringToID(name), bufferArray); }
    void CommandBuffer::SetTextureArray(const char* name, BindArray<Texture>* textureArray) { SetTextureArray(StringHashID::StringToID(name), textureArray); }

    void CommandBuffer::SetConstant(const char* name, const void* data, uint32_t size) { SetConstant(StringHashID::StringToID(name), data, size); }
    void CommandBuffer::SetKeyword(const char* name, bool value) { SetKeyword(StringHashID::StringToID(name), value); }

    void CommandBuffer::SetMesh(const Mesh* mesh)
    {
        auto& vbuffers = mesh->GetVertexBuffers();
        auto* pVBuffers = PK_STACK_ALLOC(const Buffer*, vbuffers.size());

        for (auto i = 0u; i < vbuffers.size(); ++i)
        {
            pVBuffers[i] = vbuffers[i].get();
        }

        SetVertexBuffers(pVBuffers, (uint32_t)vbuffers.size());
        SetIndexBuffer(mesh->GetIndexBuffer(), 0);
    }


    void CommandBuffer::DrawMesh(const Mesh* mesh, int32_t submesh, uint32_t instanceCount, uint32_t firstInstance)
    {
        SetMesh(mesh);
     
        auto smc = mesh->GetSubmeshCount();

        if (submesh < 0)
        {
            for (auto i = 0u; i < smc; ++i)
            {
                auto& sm = mesh->GetSubmesh(i);
                DrawIndexed(sm.indexCount, 1u, sm.firstIndex, sm.firstVertex, 0u);
            }

            return;
        }

        if (submesh >= (int)smc)
        {
            submesh = (int)smc - 1;
        }

        auto& sm = mesh->GetSubmesh(submesh);
        DrawIndexed(sm.indexCount, instanceCount, sm.firstIndex, sm.firstVertex, firstInstance);
    }

    void CommandBuffer::DrawMesh(const Mesh* mesh, int32_t submesh)
    {
        DrawMesh(mesh, submesh, 1u, 0u);
    }

    void CommandBuffer::DrawMesh(const Mesh* mesh, int32_t submesh, const Shader* shader, int32_t variantIndex)
    {
        DrawMesh(mesh, submesh, shader, 1u, 0u, variantIndex);
    }

    void CommandBuffer::DrawMesh(const Mesh* mesh, int32_t submesh, const Shader* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex)
    {
        SetShader(shader, variantIndex);
        DrawMesh(mesh, submesh, instanceCount, firstInstance);
    }

    void CommandBuffer::DrawMeshIndirect(const Mesh* mesh, const Buffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        SetMesh(mesh);
        DrawIndexedIndirect(indirectArguments, offset, drawCount, stride);
    }

    void CommandBuffer::Blit(const Shader* shader, int32_t variantIndex)
    {
        SetShader(shader, variantIndex);
        Draw(3,1,0,0);
    }

    void CommandBuffer::Blit(const Shader* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex)
    {
        SetShader(shader, variantIndex);
        Draw(3, instanceCount, 0u, firstInstance);
    }

    void CommandBuffer::Dispatch(const Shader* shader, uint3 groupCount)
    {
        SetShader(shader);
        Dispatch(groupCount);
    }

    void CommandBuffer::Dispatch(const Shader* shader, uint32_t variantIndex, uint3 groupCount)
    {
        SetShader(shader, variantIndex);
        Dispatch(groupCount);
    }

    void CommandBuffer::Barrier(const Texture* texture, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags)
    {
        Barrier(texture, {}, nullptr, srcFlags, dstFlags);
    }

    void CommandBuffer::Barrier(const Texture* texture, const TextureViewRange& range, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags)
    {
        Barrier(texture, range, nullptr, srcFlags, dstFlags);
    }

    void CommandBuffer::Barrier(const Texture* texture, uint16_t level, uint16_t layer, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags)
    {
        Barrier(texture, { level, layer, 1u, 1u }, nullptr, srcFlags, dstFlags);
    }

    void CommandBuffer::Barrier(const Buffer* buffer, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags)
    {
        Barrier(nullptr, {}, buffer, srcFlags, dstFlags);
    }

    void CommandBuffer::Barrier(MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags)
    {
        Barrier(nullptr, {}, nullptr, srcFlags, dstFlags);
    }
}
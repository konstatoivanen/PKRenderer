#include "PrecompiledHeader.h"
#include "CommandBuffer.h"

namespace PK::Rendering::Objects
{
    void CommandBuffer::SetRenderTarget(RenderTexture* renderTarget, bool updateViewPort)
    {
        auto i = 0u;
        auto count = renderTarget->GetColorCount();

        RenderTargets renderTargets;
        TextureViewRange ranges[PK_MAX_RENDER_TARGETS + 1]{};
    
        for (; i < count; ++i)
        {
            renderTargets.targets[i] = renderTarget->GetColor(i);
        }

        auto depth = renderTarget->GetDepth();

        if (depth != nullptr)
        {
            renderTargets.targets[i++] = depth;
        }

        SetRenderTarget(renderTargets.targets, nullptr, ranges, i);

        if (updateViewPort)
        {
            auto rect = renderTarget->GetRect();
            SetViewPort(rect, 0.0f, 1.0f);
            SetScissor(rect);
        }
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

    void CommandBuffer::SetRenderTarget(Texture* renderTarget, ushort level, ushort layer)
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

    void CommandBuffer::SetRenderTarget(RenderTargets& renderTargets, const RenderTargetRanges& ranges)
    {
        SetRenderTarget(renderTargets.targets, nullptr, ranges.begin(), (uint32_t)(ranges.end() - ranges.begin()));
    }

    void CommandBuffer::SetRenderTarget(RenderTargets& renderTargets, RenderTargets& resolveTargets, const RenderTargetRanges& ranges)
    {
        SetRenderTarget(renderTargets.targets, resolveTargets.targets, ranges.begin(), (uint32_t)(ranges.end() - ranges.begin()));
    }

    void CommandBuffer::SetBuffer(const char* name, const Buffer* buffer) { SetBuffer(StringHashID::StringToID(name), buffer); }

    void CommandBuffer::SetTexture(uint32_t nameHashId, Texture* texture) { SetTexture(nameHashId, texture, {}); }
    void CommandBuffer::SetTexture(uint32_t nameHashId, Texture* texture, ushort level, ushort layer) { SetTexture(nameHashId, texture, { level, layer, 1u, 1u }); }
    void CommandBuffer::SetTexture(const char* name, Texture* texture) { SetTexture(StringHashID::StringToID(name), texture); }
    void CommandBuffer::SetTexture(const char* name, Texture* texture, ushort level, ushort layer) { SetTexture(StringHashID::StringToID(name), texture, level, layer); }
    void CommandBuffer::SetTexture(const char* name, Texture* texture, const TextureViewRange& range) { SetTexture(StringHashID::StringToID(name), texture, range); }
    
    void CommandBuffer::SetImage(uint32_t nameHashId, Texture* texture) { SetImage(nameHashId, texture, {}); }
    void CommandBuffer::SetImage(uint32_t nameHashId, Texture* texture, ushort level, ushort layer) { SetImage(nameHashId, texture, { level, layer, 1u, 1u }); }
    void CommandBuffer::SetImage(const char* name, Texture* texture) { SetImage(StringHashID::StringToID(name), texture); }
    void CommandBuffer::SetImage(const char* name, Texture* texture, ushort level, ushort layer) { SetImage(StringHashID::StringToID(name), texture, level, layer); }
    void CommandBuffer::SetImage(const char* name, Texture* texture, const TextureViewRange& range) { SetImage(StringHashID::StringToID(name), texture, range); }

    void CommandBuffer::SetConstant(const char* name, const void* data, uint32_t size) { SetConstant(StringHashID::StringToID(name), data, size); }
    void CommandBuffer::SetKeyword(const char* name, bool value) { SetKeyword(StringHashID::StringToID(name), value); }

    void CommandBuffer::SetMesh(const Mesh* mesh)
    {
        auto vbuffers = mesh->GetVertexBuffers();
        auto* pVBuffers = PK_STACK_ALLOC(const Buffer*, vbuffers.size());

        for (auto i = 0u; i < vbuffers.size(); ++i)
        {
            pVBuffers[i] = vbuffers[i].get();
        }

        SetVertexBuffers(pVBuffers, (uint)vbuffers.size());
        SetIndexBuffer(mesh->GetIndexBuffer(), 0);
    }

    void CommandBuffer::DrawMesh(const Mesh* mesh, int submesh)
    {
        SetMesh(mesh);
     
        PK::Rendering::Structs::IndexRange sm;
        auto smc = mesh->GetSubmeshCount();

        if (submesh < 0)
        {
            for (auto i = 0u; i < smc; ++i)
            {
                sm = mesh->GetSubmesh(i);
                DrawIndexed(sm.count, 1, sm.offset, 0, 0);
            }

            return;
        }

        if (submesh >= (int)smc)
        {
            submesh = (int)smc - 1;
        }

        sm = mesh->GetSubmesh(submesh);
        DrawIndexed(sm.count, 1, sm.offset, 0, 0);
    }

    void CommandBuffer::DrawMesh(const Mesh* mesh, int submesh, Shader* shader, int variantIndex)
    {
        SetShader(shader, variantIndex);
        DrawMesh(mesh, submesh);
    }

    void CommandBuffer::Blit(Shader* shader, int variantIndex)
    {
        SetShader(shader, variantIndex);
        Draw(3,1,0,0);
    }

    void CommandBuffer::Dispatch(Shader* shader, uint3 groupCount)
    {
        SetShader(shader);
        Dispatch(groupCount);
    }

    void CommandBuffer::Dispatch(Shader* shader, uint variantIndex, uint3 groupCount)
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

    void CommandBuffer::Barrier(const Texture* texture, ushort level, ushort layer, MemoryAccessFlags srcFlags, MemoryAccessFlags dstFlags)
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
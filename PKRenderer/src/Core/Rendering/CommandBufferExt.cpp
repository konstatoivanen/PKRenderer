#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/Rendering/ShaderBindingTable.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/Mesh.h"
#include "CommandBufferExt.h"

namespace PK
{
    void CommandBufferExt::SetViewPort(const uint4& rect)
    {
        commandBuffer->SetViewPorts(&rect, 1);
    }

    void CommandBufferExt::SetScissor(const uint4& rect)
    {
        commandBuffer->SetScissors(&rect, 1);
    }

    void CommandBufferExt::SetFixedStateAttributes(const FixedFunctionShaderAttributes* attribs)
    {
        if (attribs == nullptr)
        {
            return;
        }

        commandBuffer->SetBlending(attribs->blending);
        commandBuffer->SetDepthStencil(attribs->depthStencil);
        commandBuffer->SetRasterization(attribs->rasterization);
    }

    void CommandBufferExt::SetShaderBindingTable(ShaderBindingTable* bindingTable)
    {
        commandBuffer->SetShaderBindingTable(RayTracingShaderGroup::RayGeneration,
            bindingTable->buffer.get(),
            bindingTable->tableInfo.byteOffsets[(uint32_t)RayTracingShaderGroup::RayGeneration],
            bindingTable->tableInfo.handleSizeAligned,
            bindingTable->tableInfo.handleSizeAligned);

        commandBuffer->SetShaderBindingTable(RayTracingShaderGroup::Miss,
            bindingTable->buffer.get(),
            bindingTable->tableInfo.byteOffsets[(uint32_t)RayTracingShaderGroup::Miss],
            bindingTable->tableInfo.handleSizeAligned,
            bindingTable->tableInfo.handleSizeAligned);

        commandBuffer->SetShaderBindingTable(RayTracingShaderGroup::Hit,
            bindingTable->buffer.get(),
            bindingTable->tableInfo.byteOffsets[(uint32_t)RayTracingShaderGroup::Hit],
            bindingTable->tableInfo.handleSizeAligned,
            bindingTable->tableInfo.handleSizeAligned);
    }

    void CommandBufferExt::SetShader(const ShaderAsset* shader, int32_t variantIndex)
    {
        if (variantIndex == -1)
        {
            auto selector = shader->GetRHISelector();
            selector.SetKeywordsFrom(*RHI::GetDriver()->GetResourceState());
            variantIndex = selector.GetIndex();
        }

        auto pVariant = shader->GetRHI(variantIndex);
        auto& fixedAttrib = shader->GetFixedFunctionAttributes();

        // No need to assign raster params for a non graphics pipeline
        if (pVariant->IsGraphics())
        {
            SetFixedStateAttributes(&fixedAttrib);
        }

        commandBuffer->SetShader(pVariant);
    }

    void CommandBufferExt::SetRenderTarget(const std::initializer_list<RHITexture*>& targets, const RenderTargetRanges& ranges, bool updateViewPort)
    {
        const uint32_t targetCount = (uint32_t)(targets.end() - targets.begin());
        const uint32_t rangeCount = (uint32_t)(ranges.end() - ranges.begin());
        PK_THROW_ASSERT(targetCount == rangeCount, "target & view range count missmatch!");

        commandBuffer->SetRenderTarget(targets.begin(), nullptr, ranges.begin(), targetCount);

        if (updateViewPort)
        {
            auto rect = targets.begin()[0]->GetRect();
            SetViewPort(rect);
            SetScissor(rect);
        }
    }

    void CommandBufferExt::SetRenderTarget(const std::initializer_list<RHITexture*>& targets, bool updateViewPort)
    {
        const uint32_t targetCount = (uint32_t)(targets.end() - targets.begin());

        // Zero ranges as they will be reset to default anyhow.
        TextureViewRange ranges[PK_RHI_MAX_RENDER_TARGETS + 1]{};

        commandBuffer->SetRenderTarget(targets.begin(), nullptr, ranges, targetCount);

        if (updateViewPort)
        {
            auto rect = targets.begin()[0]->GetRect();
            SetViewPort(rect);
            SetScissor(rect);
        }
    }

    void CommandBufferExt::SetRenderTarget(RHITexture* renderTarget)
    {
        TextureViewRange range{};
        commandBuffer->SetRenderTarget(&renderTarget, nullptr, &range, 1u);
    }

    void CommandBufferExt::SetRenderTarget(RHITexture* target, const TextureViewRange& range)
    {
        commandBuffer->SetRenderTarget(&target, nullptr, &range, 1u);
    }

    void CommandBufferExt::SetRenderTarget(RHITexture* target, uint16_t level, uint16_t layer)
    {
        TextureViewRange range = { level, layer, 1u, 1u };
        commandBuffer->SetRenderTarget(&target, nullptr, &range, 1u);
    }

    void CommandBufferExt::SetRenderTarget(RHITexture* target, const RenderTargetRanges& ranges)
    {
        auto count = (uint32_t)(ranges.end() - ranges.begin());
        auto targets = PK_STACK_ALLOC(RHITexture*, count);

        for (auto i = 0u; i < count; ++i)
        {
            targets[i] = target;
        }

        commandBuffer->SetRenderTarget(targets, nullptr, ranges.begin(), count);
    }

    void CommandBufferExt::SetVertexStreams(const VertexStreamLayout& layout)
    {
        commandBuffer->SetVertexStreams(layout.GetData(), (uint32_t)layout.GetCount());
    }

    void CommandBufferExt::ResetBuiltInAtomicCounter()
    {
        auto counter = RHIDriver::Get()->GetBuiltInResources()->AtomicCounter.get();
        commandBuffer->Clear(counter, 0, sizeof(uint32_t), 0u);
    }

    void CommandBufferExt::Blit(const ShaderAsset* shader, int32_t variantIndex)
    {
        SetShader(shader, variantIndex);
        commandBuffer->Draw(3, 1, 0, 0);
    }

    void CommandBufferExt::Blit(const ShaderAsset* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex)
    {
        SetShader(shader, variantIndex);
        commandBuffer->Draw(3, instanceCount, 0u, firstInstance);
    }

    void CommandBufferExt::Dispatch(const ShaderAsset* shader, uint3 dimensions)
    {
        SetShader(shader);
        commandBuffer->Dispatch(dimensions);
    }

    void CommandBufferExt::Dispatch(const ShaderAsset* shader, uint32_t variantIndex, uint3 dimensions)
    {
        SetShader(shader, variantIndex);
        commandBuffer->Dispatch(dimensions);
    }

    void CommandBufferExt::DispatchWithCounter(const ShaderAsset* shader, uint32_t variantIndex, uint3 dimensions)
    {
        ResetBuiltInAtomicCounter();
        SetShader(shader, variantIndex);
        commandBuffer->Dispatch(dimensions);
    }

    void CommandBufferExt::DispatchWithCounter(const ShaderAsset* shader, uint3 dimensions)
    {
        ResetBuiltInAtomicCounter();
        SetShader(shader);
        commandBuffer->Dispatch(dimensions);
    }

    void CommandBufferExt::DispatchRays(const ShaderAsset* shader, uint3 dimensions)
    {
        SetShader(shader);
        commandBuffer->DispatchRays(dimensions);
    }

    void CommandBufferExt::DispatchRays(const ShaderAsset* shader, uint32_t variantIndex, uint3 dimensions)
    {
        SetShader(shader, variantIndex);
        commandBuffer->DispatchRays(dimensions);
    }

    void CommandBufferExt::UploadBufferData(RHIBuffer* buffer, const void* data)
    {
        auto dst = commandBuffer->BeginBufferWrite(buffer, 0, buffer->GetCapacity());
        memcpy(reinterpret_cast<char*>(dst), data, buffer->GetCapacity());
        commandBuffer->EndBufferWrite(buffer);
    }

    void CommandBufferExt::UploadBufferData(RHIBuffer* buffer, const void* data, size_t offset, size_t size)
    {
        auto dst = commandBuffer->BeginBufferWrite(buffer, 0, buffer->GetCapacity());
        memcpy(reinterpret_cast<char*>(dst) + offset, data, size);
        commandBuffer->EndBufferWrite(buffer);
    }

    void CommandBufferExt::UploadBufferSubData(RHIBuffer* buffer, const void* data, size_t offset, size_t size)
    {
        auto dst = commandBuffer->BeginBufferWrite(buffer, offset, size);
        memcpy(dst, data, size);
        commandBuffer->EndBufferWrite(buffer);
    }

    void CommandBufferExt::SetMesh(const Mesh* mesh)
    {
        auto& vbuffers = mesh->GetVertexBuffers();
        const RHIBuffer* pVBuffers[PK_RHI_MAX_VERTEX_ATTRIBUTES];

        for (auto i = 0u; i < vbuffers.GetCount(); ++i)
        {
            pVBuffers[i] = vbuffers[i].get();
        }

        commandBuffer->SetVertexBuffers(pVBuffers, (uint32_t)vbuffers.GetCount());
        SetVertexStreams(mesh->GetVertexStreamLayout());
        commandBuffer->SetIndexBuffer(mesh->GetIndexBuffer(), 0, mesh->GetIndexType());
    }

    void CommandBufferExt::DrawMesh(const Mesh* mesh, int32_t submesh, uint32_t instanceCount, uint32_t firstInstance)
    {
        SetMesh(mesh);

        auto smc = mesh->GetSubmeshCount();

        if (submesh < 0)
        {
            for (auto i = 0u; i < smc; ++i)
            {
                auto& sm = mesh->GetSubmesh(i);
                commandBuffer->DrawIndexed(sm.indexCount, 1u, sm.indexFirst, sm.vertexFirst, 0u);
            }

            return;
        }

        if (submesh >= (int)smc)
        {
            submesh = (int)smc - 1;
        }

        auto& sm = mesh->GetSubmesh(submesh);
        commandBuffer->DrawIndexed(sm.indexCount, instanceCount, sm.indexFirst, sm.vertexFirst, firstInstance);
    }

    void CommandBufferExt::DrawMesh(const Mesh* mesh, int32_t submesh)
    {
        DrawMesh(mesh, submesh, 1u, 0u);
    }

    void CommandBufferExt::DrawMesh(const Mesh* mesh, int32_t submesh, const ShaderAsset* shader, int32_t variantIndex)
    {
        DrawMesh(mesh, submesh, shader, 1u, 0u, variantIndex);
    }

    void CommandBufferExt::DrawMesh(const Mesh* mesh, int32_t submesh, const ShaderAsset* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex)
    {
        SetShader(shader, variantIndex);
        DrawMesh(mesh, submesh, instanceCount, firstInstance);
    }

    void CommandBufferExt::DrawMeshIndirect(const Mesh* mesh, const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        SetMesh(mesh);
        commandBuffer->DrawIndexedIndirect(indirectArguments, offset, drawCount, stride);
    }
}
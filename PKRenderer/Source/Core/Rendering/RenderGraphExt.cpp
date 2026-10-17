#include "PrecompiledHeader.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/RHI/BuiltInResources.h"
#include "Core/Rendering/Window.h"
#include "Core/Rendering/ShaderBindingTable.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/Mesh.h"
#include "RenderGraphExt.h"

namespace PK
{
    void RenderGraphExt::SetViewPort(const uint4& rect)
    {
        graph->SetViewPorts(&rect, 1);
    }

    void RenderGraphExt::SetScissor(const uint4& rect)
    {
        graph->SetScissors(&rect, 1);
    }

    void RenderGraphExt::SetFixedStateAttributes(const FixedFunctionShaderAttributes* attribs)
    {
        if (attribs == nullptr)
        {
            return;
        }

        graph->SetBlending(attribs->blending);
        graph->SetDepthStencil(attribs->depthStencil);
        graph->SetRasterization(attribs->rasterization);
    }

    void RenderGraphExt::SetShaderBindingTable(ShaderBindingTable* bindingTable)
    {
        graph->SetShaderBindingTable(RayTracingShaderGroup::RayGeneration,
            bindingTable->buffer.get(),
            bindingTable->tableInfo.byteOffsets[(uint32_t)RayTracingShaderGroup::RayGeneration],
            bindingTable->tableInfo.handleSizeAligned,
            bindingTable->tableInfo.handleSizeAligned);

        graph->SetShaderBindingTable(RayTracingShaderGroup::Miss,
            bindingTable->buffer.get(),
            bindingTable->tableInfo.byteOffsets[(uint32_t)RayTracingShaderGroup::Miss],
            bindingTable->tableInfo.handleSizeAligned,
            bindingTable->tableInfo.handleSizeAligned);

        graph->SetShaderBindingTable(RayTracingShaderGroup::Hit,
            bindingTable->buffer.get(),
            bindingTable->tableInfo.byteOffsets[(uint32_t)RayTracingShaderGroup::Hit],
            bindingTable->tableInfo.handleSizeAligned,
            bindingTable->tableInfo.handleSizeAligned);
    }

    void RenderGraphExt::SetShader(const ShaderAsset* shader, int32_t variantIndex)
    {
        if (variantIndex == -1)
        {
            variantIndex = shader->GetRHIIndex(graph->GetResourceState());
        }

        auto pVariant = shader->GetRHI(variantIndex);
        auto& fixedAttrib = shader->GetFixedFunctionAttributes();

        // No need to assign raster params for a non graphics pipeline
        if (pVariant->IsGraphics())
        {
            SetFixedStateAttributes(&fixedAttrib);
        }

        graph->SetShader(pVariant);
    }

    void RenderGraphExt::SetRenderTarget(const initializer_list<RenderTargetBinding>& targets, bool updateViewPort)
    {
        const uint32_t targetCount = (uint32_t)(targets.end() - targets.begin());
        auto& primary = targets.begin()[0];
        auto renderArea = primary.target->GetRect();
        auto layerCount = math::min(math::max(1u, (uint32_t)primary.targetRange.layers), primary.target->GetLayers());
        graph->SetRenderTarget(targets.begin(), targetCount, renderArea, layerCount);

        if (updateViewPort)
        {
            auto rect = primary.target->GetRect();
            SetViewPort(rect);
            SetScissor(rect);
        }
    }

    void RenderGraphExt::SetRenderTarget(const RenderTargetBinding& renderTarget, bool updateViewPort)
    {
        auto renderArea = renderTarget.target->GetRect();
        auto layerCount = math::min(math::max(1u, (uint32_t)renderTarget.targetRange.layers), renderTarget.target->GetLayers());
        graph->SetRenderTarget(&renderTarget, 1u, renderArea, layerCount);

        if (updateViewPort)
        {
            auto rect = renderTarget.target->GetRect();
            SetViewPort(rect);
            SetScissor(rect);
        }
    }

    void RenderGraphExt::SetRenderTarget(const uint2& resolution, uint32_t layerCount)
    {
        graph->SetRenderTarget(nullptr, 0u, { 0,0, resolution }, layerCount);
    }

    void RenderGraphExt::SetVertexStreams(const VertexStreamLayout& layout)
    {
        graph->SetVertexStreams(layout.GetData(), (uint32_t)layout.GetCount());
    }

    void RenderGraphExt::Blit(const ShaderAsset* shader, int32_t variantIndex)
    {
        SetShader(shader, variantIndex);
        graph->Draw(3, 1, 0, 0);
    }

    void RenderGraphExt::Blit(const ShaderAsset* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex)
    {
        SetShader(shader, variantIndex);
        graph->Draw(3, instanceCount, 0u, firstInstance);
    }

    void RenderGraphExt::Dispatch(QueueType queue, const ShaderAsset* shader, uint3 dimensions)
    {
        SetShader(shader);
        graph->Dispatch(queue, dimensions);
    }

    void RenderGraphExt::Dispatch(QueueType queue, const ShaderAsset* shader, uint32_t variantIndex, uint3 dimensions)
    {
        SetShader(shader, variantIndex);
        graph->Dispatch(queue, dimensions);
    }

    void RenderGraphExt::DispatchRays(QueueType queue, const ShaderAsset* shader, uint3 dimensions)
    {
        SetShader(shader);
        graph->DispatchRays(queue, dimensions);
    }

    void RenderGraphExt::DispatchRays(QueueType queue, const ShaderAsset* shader, uint32_t variantIndex, uint3 dimensions)
    {
        SetShader(shader, variantIndex);
        graph->DispatchRays(queue, dimensions);
    }

    void RenderGraphExt::UploadBufferData(QueueType queue, RHIBuffer* buffer, const void* data, size_t offset, size_t size)
    {
        offset = size ? offset : 0ull;
        size = size ? size : buffer->GetSize();
        auto stage = graph->AcquireStagingBuffer(size);
        memcpy(stage->BeginMap(0ull,0ull), data, size);
        graph->CopyBuffer(queue, buffer, stage, 0ull, offset, size);
        graph->ReleaseStagingBuffer(stage);
    }

    void RenderGraphExt::UploadTexture(QueueType queue, RHITexture* texture, const void* data, size_t size, TextureDataRegion* regions, uint32_t regionCount)
    {
        auto stage = graph->AcquireStagingBuffer(size);
        memcpy(stage->BeginMap(0ull,0ull), data, size);
        graph->CopyToTexture(queue, texture, stage, regions, regionCount);
        graph->ReleaseStagingBuffer(stage);
    }

    void RenderGraphExt::UploadTexture(QueueType queue, RHITexture* texture, const void* data, size_t size, uint32_t level, uint32_t layer, uint32_t layers)
    {
        TextureDataRegion region;
        region.bufferOffset = 0ull;
        region.level = level;
        region.layer = layer;
        region.layers = layers;
        region.offset = PK_UINT3_ZERO;
        region.extent = texture->GetResolution();
        UploadTexture(queue, texture, data, size, &region, 1u);
    }

    void RenderGraphExt::SetMesh(const IMesh* mesh)
    {
        auto& vbuffers = mesh->GetVertexBuffers();
        const RHIBuffer* pVBuffers[PK_RHI_MAX_VERTEX_ATTRIBUTES];

        for (auto i = 0u; i < vbuffers.GetCount(); ++i)
        {
            pVBuffers[i] = vbuffers[i].get();
        }

        graph->SetIndexBuffer(mesh->GetIndexBuffer(), mesh->GetIndexSize());
        graph->SetVertexBuffers(pVBuffers, (uint32_t)vbuffers.GetCount());
        SetVertexStreams(mesh->GetVertexStreamLayout());
    }

    void RenderGraphExt::DrawMesh(const IMesh* mesh, int32_t submesh, uint32_t instanceCount, uint32_t firstInstance)
    {
        SetMesh(mesh);

        auto smc = mesh->GetSubmeshCount();

        if (submesh < 0)
        {
            for (auto i = 0u; i < smc; ++i)
            {
                auto& sm = mesh->GetSubmesh(i);
                graph->DrawIndexed(sm.indexCount, 1u, sm.indexFirst, sm.vertexFirst, 0u);
            }

            return;
        }

        if (submesh >= (int)smc)
        {
            submesh = (int)smc - 1;
        }

        auto& sm = mesh->GetSubmesh(submesh);
        graph->DrawIndexed(sm.indexCount, instanceCount, sm.indexFirst, sm.vertexFirst, firstInstance);
    }

    void RenderGraphExt::DrawMesh(const IMesh* mesh, int32_t submesh)
    {
        DrawMesh(mesh, submesh, 1u, 0u);
    }

    void RenderGraphExt::DrawMesh(const IMesh* mesh, int32_t submesh, const ShaderAsset* shader, int32_t variantIndex)
    {
        DrawMesh(mesh, submesh, shader, 1u, 0u, variantIndex);
    }

    void RenderGraphExt::DrawMesh(const IMesh* mesh, int32_t submesh, const ShaderAsset* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex)
    {
        SetShader(shader, variantIndex);
        DrawMesh(mesh, submesh, instanceCount, firstInstance);
    }

    void RenderGraphExt::DrawMeshIndirect(const IMesh* mesh, const RHIBuffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        SetMesh(mesh);
        graph->DrawIndexedIndirect(indirectArguments, offset, drawCount, stride);
    }
    
    void RenderGraphExt::BeginStatScope(NameID name, const float4& color)
    {
        graph->BeginDebugScope(name.c_str(), color);
        graph->BeginTimer(name);
    }

    void RenderGraphExt::EndStatScope()
    {
        graph->EndTimer();
        graph->EndDebugScope();
    }
}

#include "PrecompiledHeader.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "Rendering/Objects/Mesh.h"
#include "RenderingUtility.h"

namespace PK::Rendering::Geometry
{
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    void SetMesh(CommandBuffer* cmd, const Mesh* mesh)
    {
        auto& vbuffers = mesh->GetVertexBuffers();
        const Buffer* pVBuffers[PK_MAX_VERTEX_ATTRIBUTES];

        for (auto i = 0u; i < vbuffers.GetCount(); ++i)
        {
            pVBuffers[i] = vbuffers[i].get();
        }

        cmd->SetVertexBuffers(pVBuffers, (uint32_t)vbuffers.GetCount());
        cmd->SetIndexBuffer(mesh->GetIndexBuffer(), 0);
    }

    void DrawMesh(CommandBuffer* cmd, const Mesh* mesh, int32_t submesh, uint32_t instanceCount, uint32_t firstInstance)
    {
        SetMesh(cmd, mesh);

        auto smc = mesh->GetSubmeshCount();

        if (submesh < 0)
        {
            for (auto i = 0u; i < smc; ++i)
            {
                auto& sm = mesh->GetSubmesh(i);
                cmd->DrawIndexed(sm.indexCount, 1u, sm.firstIndex, sm.firstVertex, 0u);
            }

            return;
        }

        if (submesh >= (int)smc)
        {
            submesh = (int)smc - 1;
        }

        auto& sm = mesh->GetSubmesh(submesh);
        cmd->DrawIndexed(sm.indexCount, instanceCount, sm.firstIndex, sm.firstVertex, firstInstance);
    }

    void DrawMesh(CommandBuffer* cmd, const Mesh* mesh, int32_t submesh)
    {
        DrawMesh(cmd, mesh, submesh, 1u, 0u);
    }

    void DrawMesh(CommandBuffer* cmd, const Mesh* mesh, int32_t submesh, const Shader* shader, int32_t variantIndex)
    {
        DrawMesh(cmd, mesh, submesh, shader, 1u, 0u, variantIndex);
    }

    void DrawMesh(CommandBuffer* cmd, const Mesh* mesh, int32_t submesh, const Shader* shader, uint32_t instanceCount, uint32_t firstInstance, int32_t variantIndex)
    {
        cmd->SetShader(shader, variantIndex);
        DrawMesh(cmd, mesh, submesh, instanceCount, firstInstance);
    }

    void DrawMeshIndirect(CommandBuffer* cmd, const Mesh* mesh, const Buffer* indirectArguments, size_t offset, uint32_t drawCount, uint32_t stride)
    {
        SetMesh(cmd, mesh);
        cmd->DrawIndexedIndirect(indirectArguments, offset, drawCount, stride);
    }
}
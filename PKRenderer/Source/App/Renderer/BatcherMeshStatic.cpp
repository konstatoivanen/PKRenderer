#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMatrix.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/ShaderAsset.h"
#include "Core/Rendering/Material.h"
#include "App/ECS/ComponentTransform.h"
#include "App/Renderer/HashCache.h"
#include "BatcherMeshStatic.h"

namespace PK::App
{
    BatcherMeshStatic::BatcherMeshStatic() :
        m_textures2D(PK_RHI_MAX_UNBOUNDED_SIZE),
        m_transforms(1024u, 3u)
    {
        PK_LOG_VERBOSE_FUNC("");

        m_staticGeometry = CreateUnique<MeshStaticCollection>();
        m_matrices = RHI::CreateBuffer<float3x4>(1024ull, BufferUsage::PersistentStorage, "Batching.Matrices");
        m_indices = RHI::CreateBuffer<PKAssets::PKDrawInfo>(1024ull, BufferUsage::PersistentStorage, "Batching.DrawInfos");
        m_properties = RHI::CreateBuffer(16384ull, BufferUsage::PersistentStorage, "Batching.MaterialProperties");
        m_tasklets = RHI::CreateBuffer<uint2>(4096u, BufferUsage::PersistentStorage, "Batching.Meshlet.Tasklets");
        m_drawInfos.reserve(1024);
        m_drawCalls.reserve(256);
        m_passGroups.reserve(256);
    }

    void BatcherMeshStatic::BeginCollectDrawCalls()
    {
        for (auto i = 0u; i < m_shaders.GetCount(); ++i)
        {
            m_shaders[i].materialFirstIndex = 0ull;
            m_shaders[i].materialCount = 0ull;
        }

        m_taskletCount = 0u;
        m_groupIndex = 0u;
        m_materials.ClearFast();
        m_transforms.ClearFast();
        m_textures2D.Clear();
        m_drawInfos.clear();
        m_passGroups.clear();
        m_drawCalls.clear();
    }

    void BatcherMeshStatic::UploadTransforms(CommandBufferExt cmd)
    {
        RHI::ValidateBuffer<float3x4>(m_matrices, m_transforms.GetCapacity());
        auto matrixView = cmd.BeginBufferWrite<float3x4>(m_matrices.get(), 0u, m_transforms.GetCount());

        for (auto& view : m_transforms)
        {
            matrixView[view.index] = (*view)->localToWorld;
        }

        cmd->EndBufferWrite(m_matrices.get());
    }

    void BatcherMeshStatic::UploadMaterials(CommandBufferExt cmd)
    {
        auto buffsize = 0ull;

        for (auto i = 0u; i < m_shaders.GetCount(); ++i)
        {
            const auto shaderBatch = &m_shaders[i];

            if (shaderBatch->GetSize() > 0)
            {
                shaderBatch->materialFirstIndex = (size_t)ceil((double)buffsize / shaderBatch->materialStride);
                buffsize = shaderBatch->GetOffset();
                buffsize += shaderBatch->GetSize();
            }
        }

        if (buffsize > 0)
        {
            RHI::ValidateBuffer(m_properties, buffsize);
            auto propertyView = cmd.BeginBufferWrite<char>(m_properties.get(), 0ull, buffsize);

            for (auto i = 0u; i < m_materials.GetCount(); ++i)
            {
                const auto materialRef = &m_materials[i];
                const auto shaderBatch = &m_shaders[materialRef->shaderIndex];

                if (shaderBatch->GetSize() > 0)
                {
                    auto destination = propertyView.data + shaderBatch->GetOffset();
                    materialRef->reference->CopyTo(destination + materialRef->batchIndex * shaderBatch->materialStride, &m_textures2D);
                }
            }

            cmd->EndBufferWrite(m_properties.get());
        }
    }

    void BatcherMeshStatic::UploadDrawIndices(CommandBufferExt cmd)
    {
        RHI::ValidateBuffer<PKAssets::PKDrawInfo>(m_indices, m_drawInfos.capacity());
        RHI::ValidateBuffer<uint2>(m_tasklets, m_taskletCount);

        auto taskletView = cmd.BeginBufferWrite<uint2>(m_tasklets.get(), 0u, m_taskletCount);
        auto indexView = cmd.BeginBufferWrite<PKAssets::PKDrawInfo>(m_indices.get(), 0u, m_drawInfos.size());
        auto lastInfo = &m_drawInfos[0];

        auto taskletCount = 0u;
        auto taskletPassStart = 0u;
        auto taskletDrawStart = 0u;

        for (auto i = 0u; i < m_drawInfos.size(); ++i)
        {
            const auto info = m_drawInfos.data() + i;

            // Begin new groups and/or draw calls if unbatchable
            {
                if (info->group != lastInfo->group || info->shader != lastInfo->shader)
                {
                    m_drawCalls.push_back({ m_shaders[lastInfo->shader].reference, { taskletDrawStart, taskletCount - taskletDrawStart } });
                    taskletDrawStart = taskletCount;
                }

                if (info->group != lastInfo->group)
                {
                    auto drawCount = m_drawCalls.size();
                    m_passGroups.push_back({ taskletPassStart, drawCount - taskletPassStart });
                    taskletPassStart = (uint32_t)drawCount;
                }

                lastInfo = info;
            }

            // Write draw info & tasklet buffers.
            {
                auto submesh = m_staticGeometry->GetSubmesh(info->submesh);
                auto taskCount = (submesh->meshletCount + PK_RHI_MAX_MESHLETS_PER_TASK - 1u) / PK_RHI_MAX_MESHLETS_PER_TASK;

                indexView[i] = PKAssets::PackPKDrawInfo
                (
                    (uint32_t)info->material + (uint32_t)m_shaders[info->shader].materialFirstIndex,
                    m_transforms[info->transform]->minUniformScale,
                    info->transform, 
                    info->submesh, 
                    info->userdata
                );

                for (auto j = 0u; j < taskCount; ++j)
                {
                    auto taskletMeshletCount = glm::min(PK_RHI_MAX_MESHLETS_PER_TASK, submesh->meshletCount - j * PK_RHI_MAX_MESHLETS_PER_TASK);
                    taskletView[taskletCount++] =
                    {
                        submesh->meshletFirst + j * PK_RHI_MAX_MESHLETS_PER_TASK,
                        (i & 0xFFFFFFu) | ((taskletMeshletCount & 0xFF) << 24u)
                    };
                }
            }
        }

        m_drawCalls.push_back({ m_shaders[lastInfo->shader].reference, { taskletDrawStart, taskletCount - taskletDrawStart} });
        m_passGroups.push_back({ taskletPassStart, m_drawCalls.size() - taskletPassStart });

        cmd->EndBufferWrite(m_indices.get());
        cmd->EndBufferWrite(m_tasklets.get());
    }

    void BatcherMeshStatic::EndCollectDrawCalls(CommandBufferExt cmd)
    {
        if (m_drawInfos.size() == 0)
        {
            return;
        }

        std::sort(m_drawInfos.begin(), m_drawInfos.end());

        UploadTransforms(cmd);
        UploadMaterials(cmd);
        UploadDrawIndices(cmd);

        auto hash = HashCache::Get();
        RHI::SetBuffer(hash->pk_Meshlet_Tasklets, m_tasklets.get());
        RHI::SetBuffer(hash->pk_Instancing_Transforms, m_matrices.get());
        RHI::SetBuffer(hash->pk_Instancing_Indices, m_indices.get());
        RHI::SetBuffer(hash->pk_Instancing_Properties, m_properties.get());
        RHI::SetTextureArray(hash->pk_Instancing_Textures2D, m_textures2D);
    }

    void BatcherMeshStatic::SubmitMeshStaticDraw(ComponentTransform* transform,
        ShaderAsset* shader,
        Material* material,
        MeshStatic* mesh,
        uint16_t submesh,
        uint32_t userdata,
        uint16_t sortDepth)
    {
        PK_THROW_ASSERT(mesh->baseMesh == m_staticGeometry.get(), "Cannot submit draws for meshes not registered in the scene mesh of this geometry batcher!");

        DrawInfo info{};
        info.shader = m_shaders.Add({ shader, 0ull, 0ull });
        info.material = 0u;
        info.transform = m_transforms.Add(transform);
        info.submesh = mesh->GetGlobalSubmeshIndex(submesh);
        info.userdata = userdata;
        info.group = m_groupIndex - 1;
        info.sortDepth = sortDepth;

        if (material)
        {
            auto& shaderBatch = m_shaders[info.shader];

            if (shaderBatch.materialStride == 0ull)
            {
                shaderBatch.materialStride = material->GetShader()->GetMaterialPropertyLayout().GetPaddedStride();
            }

            auto materialIndex = 0u;
            if (m_materials.Add({ material, 0ull, 0ull }, &materialIndex))
            {
                m_materials[materialIndex].batchIndex = shaderBatch.materialCount++;
                m_materials[materialIndex].shaderIndex = info.shader;
            }

            info.material = m_materials[materialIndex].batchIndex;
        }

        m_drawInfos.push_back(info);

        auto meshletCount = mesh->GetSubmesh(submesh)->meshletCount;
        m_taskletCount += (meshletCount + PK_RHI_MAX_MESHLETS_PER_TASK - 1u) / PK_RHI_MAX_MESHLETS_PER_TASK;
    }

    bool BatcherMeshStatic::RenderGroup(CommandBufferExt cmd, uint32_t group, FixedFunctionShaderAttributes* overrideAttributes, uint32_t requireKeyword)
    {
        if (group >= m_passGroups.size())
        {
            return true;
        }

        if (requireKeyword > 0u)
        {
            RHI::SetKeyword(requireKeyword, true);
        }

        auto hash = HashCache::Get();
        RHI::SetBuffer(hash->pk_Meshlet_Submeshes, m_staticGeometry->GetMeshletSubmeshBuffer());
        RHI::SetBuffer(hash->pk_Meshlets, m_staticGeometry->GetMeshletBuffer());
        RHI::SetBuffer(hash->pk_Meshlet_Vertices, m_staticGeometry->GetMeshletVertexBuffer());
        RHI::SetBuffer(hash->pk_Meshlet_Indices, m_staticGeometry->GetMeshletIndexBuffer());

        auto& passGroup = m_passGroups.at(group);
        auto start = passGroup.offset;
        auto end = passGroup.offset + passGroup.count;

        for (auto i = start; i < end; ++i)
        {
            auto& dc = m_drawCalls.at(i);
            auto shader = dc.shader;

            if (requireKeyword == 0u || shader->SupportsKeyword(requireKeyword))
            {
                RHI::SetConstant<uint>(hash->pk_Meshlet_DispatchOffset, (uint32_t)dc.indices.offset);
                cmd.SetShader(shader);
                cmd.SetFixedStateAttributes(overrideAttributes);
                cmd->DrawMeshTasks({ (uint32_t)dc.indices.count, 1u, 1u });
            }
        }

        if (requireKeyword > 0u)
        {
            RHI::SetKeyword(requireKeyword, false);
        }

        return true;
    }
}
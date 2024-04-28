#include "PrecompiledHeader.h"
#include "Math/FunctionsMatrix.h"
#include "ECS/ComponentTransform.h"
#include "Rendering/RHI/Objects/Shader.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "Rendering/RHI/GraphicsAPI.h"
#include "Rendering/Objects/Material.h"
#include "Rendering/HashCache.h"
#include "BatcherStaticMesh.h"

namespace PK::Rendering::Geometry
{
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Core::Services;
    using namespace PK::Core::Assets;
    using namespace PK::ECS;
    using namespace PK::Rendering;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    struct PK_Draw
    {
        Math::uint material;
        Math::uint transfrom;
        Math::uint mesh;
        Math::uint userdata;
    };

    constexpr const static uint32_t PK_MAX_MESHLETS_PER_TASK = 32u;

    uint16_t BatcherStaticMesh::MaterialGroup::Add(Material* material)
    {
        if (stride == 0ull)
        {
            stride = material->GetShader()->GetMaterialPropertyLayout().GetPaddedStride();
        }

        return (uint16_t)materials.Add(material);
    }

    BatcherStaticMesh::BatcherStaticMesh(AssetDatabase* assetDatabase) :
        m_textures2D(PK_MAX_UNBOUNDED_SIZE),
        m_shaders(32),
        m_transforms(1024)
    {
        PK_LOG_VERBOSE("StaticDrawBatcher.Ctor");
        PK_LOG_SCOPE_INDENT(local);

        m_staticGeometry = PK::Utilities::CreateScope<StaticSceneMesh>();

        m_matrices = Buffer::Create(ElementType::Float3x4, 1024, BufferUsage::PersistentStorage, "Batching.Matrices");

        m_indices = Buffer::Create(
            {
                {ElementType::Uint, "transform"},
                {ElementType::Uint, "material"},
                {ElementType::Uint, "mesh"},
                {ElementType::Uint, "userdata"}
            },
            1024, BufferUsage::PersistentStorage, "Batching.DrawInfos");

        m_properties = Buffer::Create(
            {
                { ElementType::Uint, "DATA"},
            },
            4096, BufferUsage::PersistentStorage, "Batching.MaterialProperties");

        m_tasklets = Buffer::Create(ElementType::Uint2, 4096u, BufferUsage::PersistentStorage, "Batching.Meshlet.Tasklets");

        m_drawCalls.reserve(512);
        m_passGroups.reserve(512);
    }

    void BatcherStaticMesh::BeginCollectDrawCalls()
    {
        for (auto i = 0u; i < m_materials.GetCount(); ++i)
        {
            m_materials[i]->Clear();
        }

        m_taskletCount = 0u;
        m_groupIndex = 0u;
        m_transforms.Clear();
        m_textures2D.Clear();
        m_drawInfos.clear();
        m_passGroups.clear();
        m_drawCalls.clear();
    }

    void BatcherStaticMesh::UploadTransforms(CommandBuffer* cmd)
    {
        m_matrices->Validate(m_transforms.GetCapacity());
        auto matrixView = cmd->BeginBufferWrite<float3x4>(m_matrices.get(), 0u, m_transforms.GetCount());

        for (auto& view : m_transforms)
        {
            matrixView[view.index] = Functions::TransposeTo3x4((*view)->localToWorld);
        }

        cmd->EndBufferWrite(m_matrices.get());
    }

    void BatcherStaticMesh::UploadMaterials(CommandBuffer* cmd)
    {
        auto buffsize = 0ull;

        for (auto& group : m_materials)
        {
            auto size = group->GetSize();

            if (size > 0)
            {
                group->firstIndex = (size_t)ceil((double)buffsize / group->stride);
                buffsize = group->firstIndex * group->stride;
                buffsize += size;
            }
        }

        if (buffsize > 0)
        {
            m_properties->Validate(buffsize / sizeof(uint32_t));
            auto propertyView = cmd->BeginBufferWrite<char>(m_properties.get(), 0ull, buffsize);

            for (auto& group : m_materials)
            {
                if (group->GetSize() > 0)
                {
                    auto destination = propertyView.data + group->GetOffset();

                    for (auto& material : group->materials)
                    {
                        (*material)->CopyTo(destination + material.index * group->stride, &m_textures2D);
                    }
                }
            }

            cmd->EndBufferWrite(m_properties.get());
        }
    }

    void BatcherStaticMesh::UploadDrawIndices(CommandBuffer* cmd)
    {
        m_indices->Validate(m_drawInfos.capacity());
        m_tasklets->Validate(m_taskletCount);
        auto taskletView = cmd->BeginBufferWrite<uint2>(m_tasklets.get(), 0u, m_taskletCount);
        auto indexView = cmd->BeginBufferWrite<PK_Draw>(m_indices.get(), 0u, m_drawInfos.size());
        auto current = m_drawInfos[0];

        // Meshlet Debug
        auto taskletCount = 0u;
        auto taskletPassStart = 0u;
        auto taskletDrawStart = 0u;

        for (auto i = 0u; i < m_drawInfos.size(); ++i)
        {
            const auto info = m_drawInfos.data() + i;
            indexView[i].material = (uint32_t)info->material + (uint32_t)m_materials[info->shader]->firstIndex;
            indexView[i].transfrom = info->transform;
            indexView[i].mesh = info->submesh;
            indexView[i].userdata = info->userdata;

            // Meshlet Debug
            {
                if (info->group != current.group || info->shader != current.shader)
                {
                    m_drawCalls.push_back({ m_shaders[current.shader], { taskletDrawStart, taskletCount - taskletDrawStart } });
                    taskletDrawStart = taskletCount;
                }

                if (info->group != current.group)
                {
                    auto drawCount = m_drawCalls.size();
                    m_passGroups.push_back({ taskletPassStart, drawCount - taskletPassStart });
                    taskletPassStart = (uint32_t)drawCount;
                }

                auto sm = m_staticGeometry->GetSubmesh(info->submesh);
                auto taskCount = (sm->meshletCount + PK_MAX_MESHLETS_PER_TASK - 1u) / PK_MAX_MESHLETS_PER_TASK;

                for (auto j = 0u; j < taskCount; ++j)
                {
                    auto taskletMeshletCount = glm::min(PK_MAX_MESHLETS_PER_TASK, sm->meshletCount - j * PK_MAX_MESHLETS_PER_TASK);
                    taskletView[taskletCount++] =
                    {
                        sm->meshletFirst + j * PK_MAX_MESHLETS_PER_TASK,
                        (i & 0xFFFFFFu) | ((taskletMeshletCount & 0xFF) << 24u)
                    };
                }
            }

            if (!DrawInfo::IsBatchable(*info, current))
            {
                current = *info;
            }
        }

        m_drawCalls.push_back({ m_shaders[current.shader], { taskletDrawStart, taskletCount - taskletDrawStart} });
        m_passGroups.push_back({ taskletPassStart, m_drawCalls.size() - taskletPassStart });

        cmd->EndBufferWrite(m_indices.get());
        cmd->EndBufferWrite(m_tasklets.get());
    }

    void BatcherStaticMesh::EndCollectDrawCalls(CommandBuffer* cmd)
    {
        if (m_drawInfos.size() == 0)
        {
            return;
        }

        std::sort(m_drawInfos.begin(), m_drawInfos.end());

        auto indirectCount = 0u;

        UploadTransforms(cmd);
        UploadMaterials(cmd);
        UploadDrawIndices(cmd);

        auto hash = HashCache::Get();
        GraphicsAPI::SetBuffer(hash->pk_Meshlet_Tasklets, m_tasklets.get());
        GraphicsAPI::SetBuffer(hash->pk_Instancing_Transforms, m_matrices.get());
        GraphicsAPI::SetBuffer(hash->pk_Instancing_Indices, m_indices.get());
        GraphicsAPI::SetBuffer(hash->pk_Instancing_Properties, m_properties.get());
        GraphicsAPI::SetTextureArray(hash->pk_Instancing_Textures2D, m_textures2D);
    }

    void BatcherStaticMesh::SubmitStaticMeshDraw(ComponentTransform* transform,
        Shader* shader,
        Material* material,
        StaticMesh* mesh,
        uint16_t submesh,
        uint32_t userdata,
        uint16_t sortDepth)
    {
        PK_THROW_ASSERT(mesh->baseMesh == m_staticGeometry.get(), "Cannot submit draws for meshes not registered in the scene mesh of this geometry batcher!");

        DrawInfo info{};

        info.shader = m_shaders.Add(shader);

        if (material != nullptr)
        {
            info.material = m_materials[info.shader]->Add(material);
        }

        info.transform = m_transforms.Add(transform);
        info.submesh = mesh->GetGlobalSubmeshIndex(submesh);
        info.userdata = userdata;
        info.group = m_groupIndex - 1;
        info.sortDepth = sortDepth;
        m_drawInfos.push_back(info);

        auto meshletCount = mesh->GetSubmesh(submesh)->meshletCount;
        m_taskletCount += (meshletCount + PK_MAX_MESHLETS_PER_TASK - 1u) / PK_MAX_MESHLETS_PER_TASK;
    }

    bool BatcherStaticMesh::RenderGroup(CommandBuffer* cmd, uint32_t group, FixedFunctionShaderAttributes* overrideAttributes, uint32_t requireKeyword)
    {
        if (group >= m_passGroups.size())
        {
            return true;
        }

        if (requireKeyword > 0u)
        {
            GraphicsAPI::SetKeyword(requireKeyword, true);
        }

        auto hash = HashCache::Get();
        GraphicsAPI::SetBuffer(hash->pk_Meshlet_Submeshes, m_staticGeometry->GetMeshletSubmeshBuffer());
        GraphicsAPI::SetBuffer(hash->pk_Meshlets, m_staticGeometry->GetMeshletBuffer());
        GraphicsAPI::SetBuffer(hash->pk_Meshlet_Vertices, m_staticGeometry->GetMeshletVertexBuffer());
        GraphicsAPI::SetBuffer(hash->pk_Meshlet_Indices, m_staticGeometry->GetMeshletIndexBuffer());

        auto& passGroup = m_passGroups.at(group);
        auto start = passGroup.offset;
        auto end = passGroup.offset + passGroup.count;

        for (auto i = start; i < end; ++i)
        {
            auto& dc = m_drawCalls.at(i);
            auto shader = dc.shader;

            if (requireKeyword == 0u || shader->SupportsKeyword(requireKeyword))
            {
                GraphicsAPI::SetConstant<uint>(hash->pk_Meshlet_DispatchOffset, (uint32_t)dc.indices.offset);
                cmd->SetShader(shader);
                cmd->SetFixedStateAttributes(overrideAttributes);
                cmd->DrawMeshTasks({ (uint32_t)dc.indices.count, 1u, 1u });
            }
        }

        if (requireKeyword > 0u)
        {
            GraphicsAPI::SetKeyword(requireKeyword, false);
        }

        return true;
    }
}
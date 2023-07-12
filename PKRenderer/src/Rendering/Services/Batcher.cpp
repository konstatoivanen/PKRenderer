#include "PrecompiledHeader.h"
#include "Batcher.h"
#include "Rendering/HashCache.h"
#include "Rendering/GraphicsAPI.h"
#include "Rendering/Structs/StructIndirectArguments.h"
#include "ECS/Contextual/EntityViews/MeshRenderableView.h"
#include "ECS/Contextual/EntityViews/LightRenderableView.h"
#include "Utilities/VectorUtilities.h"
#include "Math/FunctionsIntersect.h"

namespace PK::Rendering
{
    using namespace Core;
    using namespace Core::Services;
    using namespace Rendering;
    using namespace Rendering::Structs;
    using namespace Rendering::Objects;
    using namespace Math;
    using namespace ECS;
    using namespace ECS::EntityViews;

    uint16_t MaterialGroup::Add(Material* material)
    {
        if (stride == 0ull)
        {
            stride = material->GetShader()->GetMaterialPropertyLayout().GetPaddedStride();
        }

        return (uint16_t)materials.Add(material);
    }

    Batcher::Batcher() :
        m_textures2D(PK_MAX_UNBOUNDED_SIZE),
        m_meshes(32),
        m_shaders(32),
        m_transforms(1024)
    {
        m_matrices = Buffer::Create(ElementType::Float4x4, 1024, BufferUsage::PersistentStorage, "Batching.Matrices");

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

        m_indirectArguments = Buffer::Create(
            {
                { ElementType::Uint, "indexCount"},
                { ElementType::Uint, "instanceCount"},
                { ElementType::Uint, "firstIndex"},
                { ElementType::Int,  "vertexOffset"},
                { ElementType::Uint, "firstInstance"}
            },
            256, BufferUsage::PersistentStorage | BufferUsage::Indirect, "Batching.IndirectDrawArguments");

        m_drawCalls.reserve(512);
        m_passGroups.reserve(512);
    }

    void Batcher::BeginCollectDrawCalls()
    {
        for (auto i = 0u; i < m_materials.GetCount(); ++i)
        {
            m_materials[i]->Clear();
        }

        m_groupIndex = 0u;
        m_meshes.Clear();
        m_transforms.Clear();
        m_textures2D.Clear();
        m_drawInfos.clear();
        m_passGroups.clear();
        m_drawCalls.clear();
    }

    //@TODO add a readonly usage mode to buffers that only have upload writes
    // Removed partial writes as this was causing redundant barriers between small segments of the input buffers.
    void Batcher::EndCollectDrawCalls(Objects::CommandBuffer* cmd)
    {
        if (m_drawInfos.size() == 0)
        {
            return;
        }

        std::sort(m_drawInfos.begin(), m_drawInfos.end());

        m_matrices->Validate(m_transforms.GetCapacity());
        auto matrixView = cmd->BeginBufferWrite<float4x4>(m_matrices.get(), 0u, m_transforms.GetCount());

        for (auto& view : m_transforms)
        {
            matrixView[view.index] = (*view)->localToWorld;
        }

        cmd->EndBufferWrite(m_matrices.get());

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

        auto indirectCount = 1u;
        auto current = m_drawInfos[0];

        m_indices->Validate(m_drawInfos.capacity());
        auto indexView = cmd->BeginBufferWrite<PK_Draw>(m_indices.get(), 0u, m_drawInfos.size());

        for (auto i = 0u; i < m_drawInfos.size(); ++i)
        {
            const auto info = m_drawInfos.data() + i;
            indexView[i].material = (uint32_t)info->material + (uint32_t)m_materials[info->shader]->firstIndex;
            indexView[i].transfrom = info->transform;
            indexView[i].mesh = 0;
            indexView[i].userdata = info->userdata;

            if (info->group != current.group ||
                info->shader != current.shader ||
                info->mesh != current.mesh ||
                info->submesh != current.submesh)
            {
                current = *info;
                ++indirectCount;
            }
        }

        cmd->EndBufferWrite(m_indices.get());

        m_indirectArguments->Validate(indirectCount);
        auto indirectView = cmd->BeginBufferWrite<DrawIndexedIndirectCommand>(m_indirectArguments.get(), 0u, indirectCount);

        auto indirectIndex = 0u;
        auto pbase = 0ull;
        auto dbase = 0ull;
        auto ibase = 0ull;
        current = m_drawInfos[0];

        for (auto i = 0u; i < m_drawInfos.size(); ++i)
        {
            const auto info = m_drawInfos.data() + i;

            if (info->group == current.group &&
                info->mesh == current.mesh &&
                info->shader == current.shader &&
                info->submesh == current.submesh)
            {
                continue;
            }

            auto mesh = m_meshes.GetValue(current.mesh);
            auto& sm = mesh->GetSubmesh(current.submesh);
            auto indirect = &indirectView[indirectIndex++];
            indirect->indexCount = sm.indexCount;
            indirect->instanceCount = i - (uint32_t)dbase;
            indirect->firstIndex = sm.firstIndex;
            indirect->vertexOffset = sm.firstVertex;
            indirect->firstInstance = (uint32_t)dbase;
            current.submesh = info->submesh;
            dbase = i;

            if (info->group == current.group &&
                info->mesh == current.mesh &&
                info->shader == current.shader)
            {
                continue;
            }

            m_drawCalls.push_back({ mesh, m_shaders[current.shader], { ibase, indirectIndex - ibase } });

            if (info->group != current.group)
            {
                m_passGroups.push_back({ pbase, m_drawCalls.size() - pbase });
                pbase = m_drawCalls.size();
            }

            ibase = indirectIndex;
            current = *info;
        }

        auto& lastsm = m_meshes.GetValue(current.mesh)->GetSubmesh(current.submesh);
        indirectView[indirectIndex].indexCount = lastsm.indexCount;
        indirectView[indirectIndex].instanceCount = (uint32_t)m_drawInfos.size() - (uint32_t)dbase;
        indirectView[indirectIndex].firstIndex = lastsm.firstIndex;
        indirectView[indirectIndex].vertexOffset = lastsm.firstVertex;
        indirectView[indirectIndex++].firstInstance = (uint32_t)dbase;

        m_drawCalls.push_back({ m_meshes[current.mesh], m_shaders[current.shader], { ibase, indirectIndex - ibase } });
        m_passGroups.push_back({ pbase, m_drawCalls.size() - pbase });

        cmd->EndBufferWrite(m_indirectArguments.get());

        auto hash = HashCache::Get();
        GraphicsAPI::SetBuffer(hash->pk_Instancing_Transforms, m_matrices.get());
        GraphicsAPI::SetBuffer(hash->pk_Instancing_Indices, m_indices.get());
        GraphicsAPI::SetBuffer(hash->pk_Instancing_Properties, m_properties.get());
        GraphicsAPI::SetTextureArray(hash->pk_Instancing_Textures2D, m_textures2D);
    }

    void Batcher::SubmitDraw(Components::Transform* transform, Shader* shader, Material* material, Mesh* mesh, uint32_t submesh, uint32_t clipIndex)
    {
        DrawInfo info{};

        info.shader = m_shaders.Add(shader);

        if (material != nullptr)
        {
            info.material = m_materials[info.shader]->Add(material);
        }

        info.transform = m_transforms.Add(transform);
        info.mesh = m_meshes.Add(mesh);
        info.submesh = submesh;
        info.userdata = clipIndex;
        info.group = m_groupIndex - 1;
        m_drawInfos.push_back(info);
    }

    bool Batcher::Render(CommandBuffer* cmd, uint32_t group, FixedFunctionShaderAttributes* overrideAttributes, uint32_t requireKeyword)
    {
        if (group >= m_passGroups.size())
        {
            return false;
        }

        if (requireKeyword > 0u)
        {
            GraphicsAPI::SetKeyword(requireKeyword, true);
        }

        auto hash = HashCache::Get();
        auto& passGroup = m_passGroups.at(group);
        auto start = passGroup.offset;
        auto end = passGroup.offset + passGroup.count;
        auto stride = sizeof(DrawIndexedIndirectCommand);

        for (auto i = start; i < end; ++i)
        {
            auto& dc = m_drawCalls.at(i);
            auto shader = dc.shader;

            if (requireKeyword > 0u && !shader->SupportsKeyword(requireKeyword))
            {
                continue;
            }

            auto offset = dc.indices.offset * stride;

            cmd->SetShader(shader);
            cmd->SetFixedStateAttributes(overrideAttributes);
            cmd->DrawMeshIndirect(dc.mesh, m_indirectArguments.get(), offset, (uint32_t)dc.indices.count, (uint32_t)stride);
        }

        if (requireKeyword > 0u)
        {
            GraphicsAPI::SetKeyword(requireKeyword, false);
        }

        return true;
    }
}
#include "PrecompiledHeader.h"
#include "Batcher.h"
#include "Rendering/HashCache.h"
#include "ECS/Contextual/EntityViews/MeshRenderableView.h"
#include "ECS/Contextual/EntityViews/LightRenderableView.h"
#include "Math/FunctionsIntersect.h"
#include "Rendering/GraphicsAPI.h"
#include "Utilities/VectorUtilities.h"

using namespace PK::Rendering;
using namespace PK::ECS::EntityViews;

template<>
struct PK::Utilities::Vector::Comparer<DrawInfo>
{
    int operator()(DrawInfo& a, DrawInfo& b)
    {
        if (a.group > b.group) return 1;
        if (a.group < b.group) return -1;

        if (a.shader > b.shader) return 1;
        if (a.shader < b.shader) return -1;

        if (a.mesh > b.mesh) return 1;
        if (a.mesh < b.mesh) return -1;

        if (a.submesh > b.submesh) return 1;
        if (a.submesh < b.submesh) return -1;

        if (a.clipIndex > b.clipIndex) return 1;
        if (a.clipIndex < b.clipIndex) return -1; 

        if (a.material > b.material) return 1;
        if (a.material < b.material) return -1;

        if (a.transform > b.transform) return 1;
        if (a.transform < b.transform) return -1;

        return 0;
    }
};

namespace PK::Rendering
{
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
        m_matrices = Buffer::CreateStorage(
        { 
            {ElementType::Float4x4, "localToWorld"}, 
        }, 
        1024, BufferUsage::PersistentStage);

        m_indices = Buffer::CreateStorage(
        {
            {ElementType::Uint, "transform"},
            {ElementType::Uint, "material"},
            {ElementType::Uint, "mesh"},
            {ElementType::Uint, "clipInfo"}
        },
        1024, BufferUsage::PersistentStage);

        m_properties = Buffer::CreateStorage(
        {
            { ElementType::Uint, "DATA"},
        },
        4096, BufferUsage::PersistentStage);

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
        m_drawInfos.clear();
        m_transforms.Clear();
        m_textures2D.Clear();
        m_passGroups.clear();
        m_drawCalls.clear();
    }

    void Batcher::EndCollectDrawCalls()
    {
        if (m_drawInfos.size() == 0)
        {
            return;
        }

        Vector::QuickSort(m_drawInfos);

        m_matrices->Validate(m_transforms.GetCapacity());
        auto matrixView = m_matrices->BeginMap<float4x4>(0, m_transforms.GetCount());

        for (auto& view : m_transforms)
        {
            matrixView[view.index] = (*view)->localToWorld;
        }

        m_matrices->EndMap();

        auto alignment = GraphicsAPI::GetBufferOffsetAlignment(BufferUsage::Storage);
        auto buffsize = 0ull;

        for (auto& material : m_materials)
        {
            auto size = material->GetSize();

            if (size > 0)
            {
                buffsize = (size_t)ceil((double)buffsize / alignment) * alignment;
                material->offset = buffsize;
                buffsize += size;
            }
        }

        if (buffsize > 0)
        {
            m_properties->Validate(buffsize / sizeof(uint32_t));
            auto propertyView = m_properties->BeginMap<char>(0ull, buffsize);

            for (auto& group : m_materials)
            {
                if (group->GetSize() > 0)
                {
                    auto destination = propertyView.data + group->offset;

                    for (auto& material : group->materials)
                    {
                        (*material)->CopyTo(destination + material.index * group->stride, &m_textures2D);
                    }
                }
            }

            m_properties->EndMap();
        }

        m_indices->Validate(m_drawInfos.capacity());
        auto indexView = m_indices->BeginMap<PK_Draw>(0, m_drawInfos.size());

        auto pbase = 0ull;
        auto dbase = 0ull;
        auto current = m_drawInfos[0];

        for (auto i = 0u; i < m_drawInfos.size(); ++i)
        {
            auto info = &m_drawInfos[i];

            indexView[i].material = info->material;
            indexView[i].transfrom = info->transform;
            indexView[i].mesh = 0;
            indexView[i].clipInfo = info->clipIndex;

            if (info->group == current.group &&
                info->mesh == current.mesh &&
                info->shader == current.shader &&
                info->submesh == current.submesh)
            {
                continue;
            }

            m_drawCalls.push_back(
            { 
                m_meshes.GetValue(current.mesh), 
                m_shaders.GetValue(current.shader), 
                current.submesh, 
                { dbase, i - dbase }, 
                m_materials[current.shader]->GetPropertyRange() 
            });

            if (info->group != current.group)
            {
                m_passGroups.push_back({ pbase, m_drawCalls.size() - pbase });
                pbase = m_drawCalls.size();
            }
         
            dbase = i;
            current = *info;
        }

        m_indices->EndMap();

        m_drawCalls.push_back(
        { 
            m_meshes.GetValue(current.mesh),
            m_shaders.GetValue(current.shader),
            current.submesh, 
            { dbase, m_drawInfos.size() - dbase }, 
            m_materials[current.shader]->GetPropertyRange() 
        });

        m_passGroups.push_back({ pbase, m_drawCalls.size() - pbase });

        auto cmd = GraphicsAPI::GetCommandBuffer();
        auto hash = HashCache::Get();
        cmd->SetBuffer(hash->pk_Instancing_Transforms, m_matrices.get());
        cmd->SetTextureArray(hash->pk_Instancing_Textures2D, m_textures2D);
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
        info.clipIndex = (uint8_t)clipIndex;
        info.group = m_groupIndex - 1;
        m_drawInfos.push_back(info);
    }

    void Batcher::Render(CommandBuffer* cmd, uint32_t group, FixedFunctionShaderAttributes* overrideAttributes, uint32_t requireKeyword)
    {
        if (group >= m_passGroups.size())
        {
            return;
        }

        if (requireKeyword > 0u)
        {
            cmd->SetKeyword(requireKeyword, true);
        }

        auto hash = HashCache::Get();
        auto& passGroup = m_passGroups.at(group);
        auto start = passGroup.offset;
        auto end = passGroup.offset + passGroup.count;

        for (auto i = start; i < end; ++i)
        {
            auto& dc = m_drawCalls.at(i);
            auto shader = dc.shader;
            
            if (requireKeyword > 0u && !shader->SupportsKeyword(requireKeyword))
            {
                continue;
            }

            if (dc.properties.count > 0)
            {
                cmd->SetBuffer(hash->pk_Instancing_Properties, m_properties.get(), dc.properties);
            }

            cmd->SetBuffer(hash->pk_Instancing_Indices, m_indices.get(), dc.indices);
            cmd->SetShader(shader);
            cmd->SetFixedStateAttributes(overrideAttributes);
            cmd->DrawMesh(dc.mesh, dc.submesh, (uint32_t)dc.indices.count, 0u);
        }

        if (requireKeyword > 0u)
        {
            cmd->SetKeyword(requireKeyword, false);
        }
    }
}
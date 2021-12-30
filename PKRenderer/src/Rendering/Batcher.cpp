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
        uint32_t index = 0u;

        if (!materials.Add(material, &index))
        {
            return index;
        }

        stride += material->GetShader()->GetMaterialPropertyLayout().GetAlignedStride();
        return index;
    }

    Batcher::Batcher() : m_textures2D(PK_MAX_UNBOUNDED_SIZE)
    {
        m_matrices = Buffer::CreateStorage(
        { 
            {ElementType::Float4x4, "localToWorld"}, 
            {ElementType::Float4x4, "worldToLocal"} 
        }, 
        1024);

        m_indices = Buffer::CreateStorage(
        {
            {ElementType::Uint, "transform"},
            {ElementType::Uint, "material"},
            {ElementType::Uint, "mesh"},
            {ElementType::Uint, "clipInfo"}
        },
        1024);

        m_properties = Buffer::CreateStorage(
        {
            { ElementType::Uint, "DATA"},
        },
        4096);

        m_drawCalls.reserve(512);
        m_passGroups.reserve(512);
    }
    
    void Batcher::Reset()
    {
        for (auto& kv : m_materials)
        {
            kv.second.Clear();
        }

        m_groupIndex = 0u;
        m_drawInfos.clear();
        m_transforms.Clear();
        m_materials.clear();
        m_textures2D.Clear();
        m_passGroups.clear();
        m_drawCalls.clear();
    }

    void Batcher::BuildDrawCalls()
    {
        if (m_drawInfos.size() == 0)
        {
            return;
        }

        Vector::QuickSort(m_drawInfos);

        m_matrices->Validate(m_transforms.GetCount());
        auto transfromsView = m_transforms.GetValues();
        auto matrixView = m_matrices->BeginMap<PK_Transform>(0, transfromsView.count);

        for (auto i = 0; i < transfromsView.count; ++i)
        {
            matrixView[i].localToWorld = transfromsView[i]->localToWorld;
            matrixView[i].worldToLocal = transfromsView[i]->worldToLocal;
        }

        m_matrices->EndMap();

        auto alignment = GraphicsAPI::GetBufferOffsetAlignment(BufferUsage::Storage);
        auto buffsize = 0ull;

        for (auto& kv : m_materials)
        {
            if (kv.second.stride != 0)
            {
                buffsize = (size_t)ceil((double)buffsize / alignment) * alignment;
                kv.second.offset = buffsize;
                buffsize += kv.second.stride;
            }
        }

        m_properties->Validate(buffsize / sizeof(uint32_t));
        auto propertyView = m_properties->BeginMap<char>(0ull, buffsize);

        for (auto& kv : m_materials)
        {
            auto& group = kv.second;
            auto& shader = kv.first;

            if (group.stride == 0)
            {
                continue;
            }

            auto shaderStride = shader->GetMaterialPropertyLayout().GetAlignedStride();
            auto materialsView = group.materials.GetValues();

            for (auto i = 0u; i < materialsView.count; ++i)
            {
                auto material = materialsView[i];
                material->CopyTo(propertyView.data + group.offset + i * shaderStride, &m_textures2D);
            }
        }

        m_properties->EndMap();


        m_indices->Validate(m_drawInfos.size());
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

            m_drawCalls.push_back({ current.mesh, current.shader, current.submesh, { dbase, i - dbase }, m_materials[current.shader].GetPropertyRange() });

            if (info->group != current.group)
            {
                m_passGroups.push_back({ pbase, m_drawCalls.size() - pbase });
                pbase = m_drawCalls.size();
            }
         
            dbase = i;
            current = *info;
        }

        m_indices->EndMap();
        m_drawCalls.push_back({ current.mesh, current.shader, current.submesh, { dbase, m_drawInfos.size() - dbase }, m_materials[current.shader].GetPropertyRange() });
        m_passGroups.push_back({ pbase, m_drawCalls.size() - pbase });
    }

    void Batcher::SubmitDraw(Components::Transform* transform, Shader* shader, Material* material, Mesh* mesh, uint32_t submesh, uint32_t clipIndex)
    {
        DrawInfo info{};

        if (material != nullptr)
        {
            info.material = m_materials[shader].Add(material);
        }

        info.transform = m_transforms.AddReturnIndex(transform);
        info.shader = shader;
        info.mesh = mesh;
        info.submesh = submesh;
        info.clipIndex = (uint8_t)clipIndex;
        info.group = m_groupIndex - 1;
        m_drawInfos.push_back(info);
    }

    void Batcher::Render(CommandBuffer* cmd, uint32_t group)
    {
        if (group >= m_passGroups.size())
        {
            return;
        }

        auto hash = HashCache::Get();

        auto& passGroup = m_passGroups.at(group);
        auto start = passGroup.offset;
        auto end = passGroup.offset + passGroup.count;

        cmd->SetBuffer(hash->pk_Instancing_Transforms, m_matrices.get());
        cmd->SetTextureArray(hash->pk_Instancing_Textures2D, m_textures2D);

        for (auto i = start; i < end; ++i)
        {
            auto& dc = m_drawCalls.at(i);
            cmd->SetBuffer(hash->pk_Instancing_Indices, m_indices.get(), dc.indices);
            cmd->SetBuffer(hash->pk_Instancing_Properties, m_properties.get(), dc.properties);
            cmd->DrawMesh(dc.mesh, dc.submesh, dc.shader, (uint32_t)dc.indices.count, 0u);
        }
    }
}
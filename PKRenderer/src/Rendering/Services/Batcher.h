#pragma once
#include "ECS/EntityDatabase.h"
#include "Core/Services/Sequencer.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/BindSet.h"
#include "Rendering/Objects/Material.h"
#include "Rendering/Objects/CommandBuffer.h"
#include "ECS/Tokens/CullingTokens.h"
#include "ECS/Components/Transform.h"
#include "Utilities/IndexedSet.h"
#include "Utilities/FixedList.h"

namespace PK::Rendering
{
    struct DrawCall
    {
        const Objects::Mesh* mesh = nullptr;
        const Objects::Shader* shader = nullptr;
        Structs::IndexRange indices{};
    };

    struct MaterialGroup
    {
        Utilities::IndexedSet<Objects::Material> materials;
        size_t firstIndex = 0ull;
        size_t stride = 0ull;

        MaterialGroup() : materials(32){}
        uint16_t Add(Objects::Material* material);
        inline void Clear() { firstIndex = 0ull; materials.Clear(); }
        constexpr size_t GetSize() const { return materials.GetCount() * stride; }
        constexpr size_t GetOffset() const { return firstIndex * stride; }
    };

    struct DrawInfo
    {
        uint16_t group = 0u;
        uint16_t shader = 0u;
        uint16_t mesh = 0u;
        uint16_t material = 0u;
        uint16_t transform = 0u;
        uint16_t submesh = 0u;
        uint32_t userdata = 0u;

        bool operator < (DrawInfo& b)
        {
            if (group != b.group) return group < b.group;
            if (shader != b.shader) return shader < b.shader;
            if (mesh != b.mesh) return mesh < b.mesh;
            if (submesh != b.submesh) return submesh < b.submesh;
            if (userdata != b.userdata) return userdata < b.userdata;
            if (material != b.material) return material < b.material;
            if (transform != b.transform) return transform < b.transform;
            return false;
        }
    };
    
    class Batcher : public Utilities::NoCopy
    {
        public:
            Batcher();
            void BeginCollectDrawCalls();
            void EndCollectDrawCalls(Objects::CommandBuffer* cmd);
            constexpr uint32_t BeginNewGroup() { return m_groupIndex++; }
            void SubmitDraw(ECS::Components::Transform* transform, Objects::Shader* shader, Objects::Material* material, Objects::Mesh* mesh, uint32_t submesh, uint32_t userdata);
            bool Render(Objects::CommandBuffer* cmd, uint32_t group, Structs::FixedFunctionShaderAttributes* overrideAttributes = nullptr, uint32_t requireKeyword = 0u);

        private:
            Utilities::Ref<Objects::Buffer> m_matrices;
            Utilities::Ref<Objects::Buffer> m_indices;
            Utilities::Ref<Objects::Buffer> m_properties;
            Utilities::Ref<Objects::Buffer> m_indirectArguments;
            Objects::BindSet<Objects::Texture> m_textures2D;

            std::vector<DrawCall> m_drawCalls;
            std::vector<Structs::IndexRange> m_passGroups;     
            std::vector<DrawInfo> m_drawInfos;

            Utilities::FixedList<MaterialGroup, 32> m_materials;
            Utilities::IndexedSet<Objects::Mesh> m_meshes;
            Utilities::IndexedSet<Objects::Shader> m_shaders;
            Utilities::IndexedSet<ECS::Components::Transform> m_transforms;
            uint16_t m_groupIndex = 0u;
    };
}
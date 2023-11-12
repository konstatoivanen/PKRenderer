#pragma once
#include "Utilities/IndexedSet.h"
#include "Utilities/FixedList.h"
#include "Core/Services/Sequencer.h"
#include "Core/Services/AssetDatabase.h"
#include "ECS/EntityDatabase.h"
#include "ECS/Tokens/CullingTokens.h"
#include "ECS/Tokens/BatcherToken.h"
#include "ECS/Components/Transform.h"
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/Material.h"
#include "Rendering/Objects/BindSet.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::Rendering::Passes
{
    struct DrawCall
    {
        const Rendering::Objects::Mesh* mesh = nullptr;
        const RHI::Objects::Shader* shader = nullptr;
        RHI::IndexRange indices{};
    };

    struct MaterialGroup
    {
        Utilities::IndexedSet<Rendering::Objects::Material> materials;
        size_t firstIndex = 0ull;
        size_t stride = 0ull;

        MaterialGroup() : materials(32){}
        uint16_t Add(Rendering::Objects::Material* material);
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
    
    class Batcher : public Utilities::NoCopy, public ECS::Tokens::IBatcher
    {
        public:
            Batcher(PK::Core::Services::AssetDatabase* assetDatabase);
            void BeginCollectDrawCalls();
            void EndCollectDrawCalls(RHI::Objects::CommandBuffer* cmd);
            uint32_t BeginNewGroup() final { return m_groupIndex++; }

            void SubmitDraw(ECS::Components::Transform* transform, 
                            RHI::Objects::Shader* shader, 
                            Rendering::Objects::Material* material, 
                            Rendering::Objects::Mesh* mesh, 
                            uint32_t submesh, 
                            uint32_t userdata) final;

            bool Render(RHI::Objects::CommandBuffer* cmd, 
                        uint32_t group, 
                        RHI::FixedFunctionShaderAttributes* overrideAttributes = nullptr, 
                        uint32_t requireKeyword = 0u) final;

            void DebugComputeMeshTasks(RHI::Objects::CommandBuffer* cmd, uint32_t group);
            void DebugRenderMeshlets(RHI::Objects::CommandBuffer* cmd, uint32_t group);

        private:
            RHI::Objects::BufferRef m_matrices;
            RHI::Objects::BufferRef m_indices;
            RHI::Objects::BufferRef m_properties;
            RHI::Objects::BufferRef m_indirectArguments;
            Rendering::Objects::BindSet<RHI::Objects::Texture> m_textures2D;

            std::vector<DrawCall> m_drawCalls;
            std::vector<DrawInfo> m_drawInfos;
            std::vector<RHI::IndexRange> m_passGroups;

            Utilities::FixedList<MaterialGroup, 32> m_materials;
            Utilities::IndexedSet<Rendering::Objects::Mesh> m_meshes;
            Utilities::IndexedSet<RHI::Objects::Shader> m_shaders;
            Utilities::IndexedSet<ECS::Components::Transform> m_transforms;
            uint16_t m_groupIndex = 0u;

            // Meshlet debug
            std::vector<RHI::IndexRange> m_passGroupRanges;
            RHI::Objects::BufferRef m_Meshlet_TaskDisptaches;
            RHI::Objects::BufferRef m_Meshlet_TaskDisptachCounter;
            RHI::Objects::BufferRef m_Meshlet_TaskIndices;
            RHI::Objects::Shader* m_meshletComputeTasks;
            RHI::Objects::Shader* m_meshletRender;
    };
}
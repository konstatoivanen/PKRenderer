#pragma once
#include "Utilities/IndexedSet.h"
#include "Utilities/FixedList.h"
#include "Core/Services/ServiceRegister.h"
#include "Core/Services/Sequencer.h"
#include "Core/Services/AssetDatabase.h"
#include "ECS/EntityDatabase.h"
#include "ECS/Tokens/CullingTokens.h"
#include "ECS/Tokens/BatcherToken.h"
#include "ECS/Components/Transform.h"
#include "Rendering/Objects/StaticSceneMesh.h"
#include "Rendering/Objects/Material.h"
#include "Rendering/Objects/BindSet.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::Rendering
{
    class StaticDrawBatcher : public PK::Core::Services::IService, public ECS::Tokens::IBatcher
    {
        struct DrawCall
        {
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
            uint16_t material = 0u;
            uint16_t transform = 0u;
            uint16_t submesh = 0u;
            uint16_t sortDepth = 0u;
            uint32_t userdata = 0u;

            bool operator < (DrawInfo& b)
            {
                if (group != b.group) return group < b.group;
                if (shader != b.shader) return shader < b.shader;
                if (submesh != b.submesh) return submesh < b.submesh;
                if (userdata != b.userdata) return userdata < b.userdata;
                if (material != b.material) return material < b.material;
                if (transform != b.transform) return transform < b.transform;
                if (sortDepth != b.sortDepth) return sortDepth < b.sortDepth;
                return false;
            }

            inline static bool IsBatchable(const DrawInfo& a, const DrawInfo& b)
            { 
                return a.group == b.group && a.shader == b.shader && a.submesh == b.submesh; 
            }
        };

        public:
            StaticDrawBatcher(PK::Core::Services::AssetDatabase* assetDatabase);

            inline Rendering::Objects::StaticSceneMesh* GetStaticSceneMesh() { return m_staticGeometry.get(); }

            void BeginCollectDrawCalls();
            void EndCollectDrawCalls(RHI::Objects::CommandBuffer* cmd);
            uint32_t BeginNewGroup() final { return m_groupIndex++; }

            void SubmitStaticDraw(ECS::Components::Transform* transform, 
                                  RHI::Objects::Shader* shader, 
                                  Rendering::Objects::Material* material, 
                                  Rendering::Objects::StaticMesh* mesh,
                                  uint16_t submesh, 
                                  uint32_t userdata,
                                  uint16_t sortDepth) final;

            bool RenderMeshlets(RHI::Objects::CommandBuffer* cmd, uint32_t group,
                                RHI::FixedFunctionShaderAttributes* overrideAttributes = nullptr,
                                uint32_t requireKeyword = 0u) final;

        private:
            void UploadTransforms(RHI::Objects::CommandBuffer* cmd);
            void UploadMaterials(RHI::Objects::CommandBuffer* cmd);
            void UploadDrawIndices(RHI::Objects::CommandBuffer* cmd);

            Utilities::Scope<Rendering::Objects::StaticSceneMesh> m_staticGeometry;

            RHI::Objects::BufferRef m_matrices;
            RHI::Objects::BufferRef m_indices;
            RHI::Objects::BufferRef m_properties;
            RHI::Objects::BufferRef m_tasklets;
            Rendering::Objects::BindSet<RHI::Objects::Texture> m_textures2D;

            std::vector<DrawInfo> m_drawInfos;
            std::vector<DrawCall> m_drawCalls;
            std::vector<RHI::IndexRange> m_passGroups;

            Utilities::FixedList<MaterialGroup, 32> m_materials;
            Utilities::IndexedSet<RHI::Objects::Shader> m_shaders;
            Utilities::IndexedSet<ECS::Components::Transform> m_transforms;
            uint16_t m_groupIndex = 0u;
            uint32_t m_taskletCount = 0u;
    };
}
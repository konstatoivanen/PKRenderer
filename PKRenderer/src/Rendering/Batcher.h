#pragma once
#include "ECS/EntityDatabase.h"
#include "Core/Services/Sequencer.h"
#include "Rendering/Objects/Texture.h"
#include "Rendering/Objects/Shader.h"
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/BindSet.h"
#include "Rendering/Objects/Material.h"
#include "Rendering/Objects/CommandBuffer.h"
#include "ECS/Contextual/Tokens/CullingTokens.h"
#include "ECS/Contextual/Components/Transform.h"
#include "Utilities/IndexedSet.h"
#include "Utilities/FixedList.h"

namespace PK::Rendering
{
    using namespace PK::Rendering::Objects;
    using namespace PK::ECS;

    struct DrawCall
    {
        const Mesh* mesh = nullptr;
        const Shader* shader = nullptr;
        uint32_t submesh = 0;
        IndexRange indices{};
        IndexRange properties{};
    };

    struct MaterialGroup
    {
        IndexedSet<Material> materials;
        size_t offset = 0ull;
        size_t stride = 0ull;

        MaterialGroup() : materials(32){}
        uint16_t Add(Material* material);
        inline void Clear() { offset = 0ull; materials.Clear(); }
        constexpr size_t GetSize() const { return materials.GetCount() * stride; }
        constexpr IndexRange GetPropertyRange() const { return { offset / sizeof(uint32_t), GetSize() / sizeof(uint32_t) }; }
    };

    struct DrawInfo
    {
        uint16_t group = 0u;
        uint16_t shader = 0u;
        uint16_t mesh = 0u;
        uint16_t material = 0u;
        uint16_t transform = 0u;
        uint8_t clipIndex = 0u;
        uint8_t submesh = 0u;
    };
    
    class Batcher : public PK::Core::NoCopy
    {
        public:
            Batcher();
            void BeginCollectDrawCalls();
            void EndCollectDrawCalls();
            constexpr uint32_t BeginNewGroup() { return m_groupIndex++; }
            void SubmitDraw(Components::Transform* transform, Shader* shader, Material* material, Mesh* mesh, uint32_t submesh, uint32_t clipIndex);
            void Render(CommandBuffer* cmd, uint32_t group, Shader* replacementShader = nullptr, uint32_t requireKeyword = 0u);

        private:
            Ref<Buffer> m_matrices;
            Ref<Buffer> m_indices;
            Ref<Buffer> m_properties;
            BindSet<Texture> m_textures2D;
            //Ref<BindArray<Texture>> m_textures3D;
            //Ref<BindArray<Texture>> m_texturesCube;

            std::vector<DrawCall> m_drawCalls;
            std::vector<IndexRange> m_passGroups;
            
            std::vector<DrawInfo> m_drawInfos;
            FixedList<MaterialGroup, 32> m_materials;
            IndexedSet<Mesh> m_meshes;
            IndexedSet<Shader> m_shaders;
            IndexedSet<Components::Transform> m_transforms;
            uint16_t m_groupIndex = 0u;
    };
}
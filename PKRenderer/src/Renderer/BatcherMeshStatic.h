#pragma once
#include "Utilities/FastSet.h"
#include "Utilities/FixedList.h"
#include "Core/IService.h"
#include "Graphics/MeshStaticCollection.h"
#include "Graphics/BindSet.h"
#include "Renderer/IBatcher.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Core::Assets, class AssetDatabase)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::ECS, struct ComponentTransform)

namespace PK::Renderer
{
    class BatcherMeshStatic : public Core::IService, public IBatcher
    {
        struct DrawCall
        {
            const Graphics::Shader* shader = nullptr;
            Graphics::RHI::IndexRange indices{};
        };

        struct MaterialGroup
        {
            Utilities::PointerSet<Graphics::Material> materials;
            size_t firstIndex = 0ull;
            size_t stride = 0ull;

            MaterialGroup() : materials(32) {}
            uint16_t Add(Graphics::Material* material);
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
        BatcherMeshStatic(PK::Core::Assets::AssetDatabase* assetDatabase);

        inline Graphics::MeshStaticCollection* GetMeshStaticCollection() { return m_staticGeometry.get(); }

        void BeginCollectDrawCalls() final;

        void EndCollectDrawCalls(Graphics::CommandBufferExt cmd) final;

        uint32_t BeginNewGroup() { return m_groupIndex++; }

        void SubmitMeshStaticDraw(ECS::ComponentTransform* transform,
            Graphics::Shader* shader,
            Graphics::Material* material,
            Graphics::MeshStatic* mesh,
            uint16_t submesh,
            uint32_t userdata,
            uint16_t sortDepth);

        bool RenderGroup(Graphics::CommandBufferExt cmd,
            uint32_t group,
            Graphics::RHI::FixedFunctionShaderAttributes* overrideAttributes = nullptr,
            uint32_t requireKeyword = 0u);

    private:
        void UploadTransforms(Graphics::CommandBufferExt cmd);
        void UploadMaterials(Graphics::CommandBufferExt cmd);
        void UploadDrawIndices(Graphics::CommandBufferExt cmd);

        Utilities::Scope<Graphics::MeshStaticCollection> m_staticGeometry;

        Graphics::BufferRef m_matrices;
        Graphics::BufferRef m_indices;
        Graphics::BufferRef m_properties;
        Graphics::BufferRef m_tasklets;
        Graphics::BindSet<Graphics::Texture> m_textures2D;

        std::vector<DrawInfo> m_drawInfos;
        std::vector<DrawCall> m_drawCalls;
        std::vector<Graphics::RHI::IndexRange> m_passGroups;

        Utilities::FixedList<MaterialGroup, 32> m_materials;
        Utilities::PointerSet<Graphics::Shader> m_shaders;
        Utilities::PointerSet<ECS::ComponentTransform> m_transforms;
        uint16_t m_groupIndex = 0u;
        uint32_t m_taskletCount = 0u;
    };
}
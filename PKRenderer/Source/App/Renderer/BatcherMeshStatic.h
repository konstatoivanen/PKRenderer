#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Utilities/FastSet.h"
#include "Core/Utilities/FixedList.h"
#include "Core/Rendering/MeshStaticCollection.h"
#include "Core/Rendering/BindSet.h"
#include "App/Renderer/IBatcher.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase)

namespace PK::App
{
    struct ComponentTransform;

    class BatcherMeshStatic : public IBatcher
    {
        struct DrawCall
        {
            const ShaderAsset* shader = nullptr;
            BufferIndexRange indices{};
        };

        struct MaterialGroup
        {
            PointerSet<Material> materials;
            size_t firstIndex = 0ull;
            size_t stride = 0ull;

            MaterialGroup() : materials(32) {}
            uint16_t Add(Material* material);
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
        BatcherMeshStatic();

        inline MeshStaticCollection* GetMeshStaticCollection() { return m_staticGeometry.get(); }

        void BeginCollectDrawCalls() final;

        void EndCollectDrawCalls(CommandBufferExt cmd) final;

        uint32_t BeginNewGroup() { return m_groupIndex++; }

        void SubmitMeshStaticDraw(ComponentTransform* transform,
            ShaderAsset* shader,
            Material* material,
            MeshStatic* mesh,
            uint16_t submesh,
            uint32_t userdata,
            uint16_t sortDepth);

        bool RenderGroup(CommandBufferExt cmd,
            uint32_t group,
            FixedFunctionShaderAttributes* overrideAttributes = nullptr,
            uint32_t requireKeyword = 0u);

    private:
        void UploadTransforms(CommandBufferExt cmd);
        void UploadMaterials(CommandBufferExt cmd);
        void UploadDrawIndices(CommandBufferExt cmd);

        Unique<MeshStaticCollection> m_staticGeometry;

        RHIBufferRef m_matrices;
        RHIBufferRef m_indices;
        RHIBufferRef m_properties;
        RHIBufferRef m_tasklets;
        BindSet<RHITexture> m_textures2D;

        std::vector<DrawInfo> m_drawInfos;
        std::vector<DrawCall> m_drawCalls;
        std::vector<BufferIndexRange> m_passGroups;

        FixedList<MaterialGroup, 32> m_materials;
        PointerSet<ShaderAsset> m_shaders;
        PointerSet<ComponentTransform> m_transforms;
        uint16_t m_groupIndex = 0u;
        uint32_t m_taskletCount = 0u;
    };
}
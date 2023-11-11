#pragma once
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/MeshletMesh.h"

namespace PK::Rendering::Objects
{
    class VirtualMesh : public Core::Services::Asset, 
        public Core::Services::IAssetImport<Utilities::Ref<Mesh>*, Utilities::Ref<MeshletMesh>*>
    {
        friend Utilities::Ref<VirtualMesh> Core::Services::AssetImporters::Create();

        public:
            VirtualMesh();
            VirtualMesh(Utilities::Ref<Mesh> baseMesh,
                        const SubmeshRangeData* meshData,
                        Utilities::Ref<MeshletMesh> baseMeshletMesh,
                        MeshletRangeData* meshletData);
            ~VirtualMesh();

            virtual void Import(const char* filepath, Utilities::Ref<Mesh>* baseMesh, Utilities::Ref<MeshletMesh>* baseMeshletMesh) final;
            inline Mesh* GetBaseMesh() const { return m_baseMesh.get(); }
            uint32_t GetSubmeshIndex(uint32_t submesh) const;
            inline const uint32_t GetSubmeshCount() const { return (uint32_t)m_submeshAllocation.submeshIndices.size(); }

        private:
            Utilities::Ref<Mesh> m_baseMesh = nullptr;
            Utilities::Ref<MeshletMesh> m_baseMeshletMesh = nullptr;
            SubmeshAllocation m_submeshAllocation{};
            MeshletAllocation m_meshletAllocation{};
    };
}
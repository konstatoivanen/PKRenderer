#pragma once
#include "Rendering/Objects/StaticSceneMesh.h"

namespace PK::Rendering::Objects
{
    typedef Utilities::Ref<class VirtualStaticMesh> VirtualStaticMeshRef;

    class VirtualStaticMesh : public Core::Services::Asset, public Core::Services::IAssetImport<StaticSceneMesh*&>
    {
        friend Utilities::Ref<VirtualStaticMesh> Core::Services::AssetImporters::Create();

        public:
            VirtualStaticMesh();
            VirtualStaticMesh(StaticSceneMesh* baseMesh, StaticMeshAllocationData* allocData);
            ~VirtualStaticMesh();

            virtual void Import(const char* filepath, StaticSceneMesh*& baseMesh) final;
            
            inline StaticMesh* GetStaticMesh() const { return m_staticMesh; }
            inline const StaticSubMesh* GetStaticSubmesh(uint32_t localIndex) const { return m_staticMesh->GetSubmesh(localIndex); }
            inline uint32_t GetSubmeshCount() const { return m_staticMesh->submeshCount;  }

        private:
            StaticMesh* m_staticMesh = nullptr;
    };
}
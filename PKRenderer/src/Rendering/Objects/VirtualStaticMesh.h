#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/Assets/Asset.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, class StaticSceneMesh)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, struct StaticMeshAllocationData)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, struct StaticSubMesh)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, struct StaticMesh)

namespace PK::Rendering::Objects
{
    typedef Utilities::Ref<class VirtualStaticMesh> VirtualStaticMeshRef;

    class VirtualStaticMesh : public Core::Assets::AssetWithImport<StaticSceneMesh*&>
    {
        public:
            VirtualStaticMesh();
            VirtualStaticMesh(StaticSceneMesh* baseMesh, StaticMeshAllocationData* allocData);
            ~VirtualStaticMesh();

            virtual void AssetImport(const char* filepath, StaticSceneMesh*& baseMesh) final;
            
            inline StaticMesh* GetStaticMesh() const { return m_staticMesh; }
            const StaticSubMesh* GetStaticSubmesh(uint32_t localIndex) const;
            uint32_t GetSubmeshCount() const;

        private:
            StaticMesh* m_staticMesh = nullptr;
    };
}
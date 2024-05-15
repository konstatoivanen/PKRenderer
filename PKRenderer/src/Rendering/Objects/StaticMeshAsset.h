#pragma once
#include "Utilities/ForwardDeclare.h"
#include "Core/Assets/Asset.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, class StaticMeshCollection)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, struct StaticMeshAllocationData)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, struct StaticSubMesh)
PK_FORWARD_DECLARE_IN_NAMESPACE(PK::Rendering::Objects, struct StaticMesh)

namespace PK::Rendering::Objects
{
    typedef Utilities::Ref<class StaticMeshAsset> StaticMeshAssetRef;

    class StaticMeshAsset : public Core::Assets::AssetWithImport<StaticMeshCollection*&>
    {
        public:
            StaticMeshAsset();
            StaticMeshAsset(StaticMeshCollection* baseMesh, StaticMeshAllocationData* allocData);
            ~StaticMeshAsset();

            virtual void AssetImport(const char* filepath, StaticMeshCollection*& baseMesh) final;
            
            inline StaticMesh* GetStaticMesh() const { return m_staticMesh; }
            const StaticSubMesh* GetStaticSubmesh(uint32_t localIndex) const;
            uint32_t GetSubmeshCount() const;

        private:
            StaticMesh* m_staticMesh = nullptr;
    };
}
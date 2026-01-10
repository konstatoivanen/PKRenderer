#pragma once
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Assets/Asset.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct MeshStaticAsset : public Asset
    {
        MeshStaticAsset(MeshStaticCollection* baseMesh, const char* filepath);
        MeshStaticAsset(MeshStatic* staticMesh);
        ~MeshStaticAsset();

        inline MeshStatic* GetMeshStatic() const { return m_staticMesh; }
        const SubMeshStatic* GetStaticSubmesh(uint32_t localIndex) const;
        uint32_t GetSubmeshCount() const;

        private: MeshStatic* m_staticMesh = nullptr;
    };
}

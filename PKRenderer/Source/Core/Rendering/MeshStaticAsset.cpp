#include "PrecompiledHeader.h"
#include <filesystem>
#include <PKAssets/PKAssetLoader.h>
#include "Core/Math/FunctionsIntersect.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/CLI/Log.h"
#include "Core/Rendering/MeshStaticCollection.h"
#include "MeshStaticAsset.h"

namespace PK
{
    MeshStaticAsset::MeshStaticAsset(MeshStaticCollection* baseMesh, const char* filepath)
    {
        PKAssets::PKAsset asset;

        PK_THROW_ASSERT(PKAssets::OpenAsset(filepath, &asset) == 0, "Failed to open asset at path: %s", filepath);
        PK_THROW_ASSERT(asset.header->type == PKAssets::PKAssetType::Mesh, "Trying to read a mesh from a non mesh file!")

            auto mesh = PKAssets::ReadAsMesh(&asset);
        auto base = asset.rawData;

        PK_THROW_ASSERT(mesh->vertexAttributeCount > 0, "Trying to read a mesh with 0 vertex attributes!");
        PK_THROW_ASSERT(mesh->vertexCount > 0, "Trying to read a shader with 0 vertices!");
        PK_THROW_ASSERT(mesh->indexCount > 0, "Trying to read a shader with 0 indices!");
        PK_THROW_ASSERT(mesh->submeshCount > 0, "Trying to read a shader with 0 submeshes!");

        auto pAttributes = mesh->vertexAttributes.Get(base);
        auto pVertices = mesh->vertexBuffer.Get(base);
        auto pIndices = mesh->indexBuffer.Get(base);
        auto pSubmeshes = mesh->submeshes.Get(base);

        std::vector<MeshStaticDescriptor::SubMesh> submeshes;
        submeshes.reserve(mesh->submeshCount);

        for (auto i = 0u; i < mesh->submeshCount; ++i)
        {
            auto bounds = BoundingBox::MinMax(Math::ToFloat3(pSubmeshes[i].bbmin), Math::ToFloat3(pSubmeshes[i].bbmax));
            submeshes.push_back({ 0u, mesh->vertexCount, pSubmeshes[i].firstIndex, pSubmeshes[i].indexCount, bounds });
        }

        VertexStreamLayout streamLayout;
        for (auto i = 0u; i < mesh->vertexAttributeCount; ++i)
        {
            auto stream = streamLayout.Add();
            stream->name = pAttributes[i].name;
            stream->stream = (byte)pAttributes[i].stream;
            stream->inputRate = InputRate::PerVertex;
            stream->stride = 0u;
            stream->offset = pAttributes[i].offset;
            stream->size = pAttributes[i].size;
        }

        streamLayout.CalculateOffsetsAndStride();

        {
            PK_THROW_ASSERT(baseMesh, "Cannot create a virtual mesh without a base mesh!");

            MeshStaticDescriptor desc{};
            desc.name = std::filesystem::path(GetFileName()).stem().string().c_str();

            desc.regular.pVertices = pVertices;
            desc.regular.pIndices = pIndices;
            desc.regular.streamLayout = streamLayout;
            desc.regular.pSubmeshes = submeshes.data();
            desc.regular.indexType = mesh->indexType;
            desc.regular.vertexCount = mesh->vertexCount;
            desc.regular.indexCount = mesh->indexCount;
            desc.regular.submeshCount = mesh->submeshCount;

            auto meshletMesh = mesh->meshletMesh.Get(base);
            desc.meshlet.pSubmeshes = meshletMesh->submeshes.Get(base);
            desc.meshlet.submeshCount = meshletMesh->submeshCount;
            desc.meshlet.pMeshlets = meshletMesh->meshlets.Get(base);
            desc.meshlet.meshletCount = meshletMesh->meshletCount;
            desc.meshlet.pVertices = meshletMesh->vertices.Get(base);
            desc.meshlet.vertexCount = meshletMesh->vertexCount;
            desc.meshlet.pIndices = meshletMesh->indices.Get(base);
            desc.meshlet.triangleCount = meshletMesh->triangleCount;

            m_staticMesh = baseMesh->Allocate(&desc);
        }

        PKAssets::CloseAsset(&asset);
    }

    MeshStaticAsset::MeshStaticAsset(MeshStatic* staticMesh)
    {
        m_staticMesh = staticMesh;
    }

    MeshStaticAsset::~MeshStaticAsset() 
    { 
        m_staticMesh->baseMesh->Deallocate(m_staticMesh); 
    }

    const SubMeshStatic* MeshStaticAsset::GetStaticSubmesh(uint32_t localIndex) const 
    { 
        return m_staticMesh->GetSubmesh(localIndex); 
    }

    uint32_t MeshStaticAsset::GetSubmeshCount() const 
    { 
        return m_staticMesh->submeshCount; 
    }
}


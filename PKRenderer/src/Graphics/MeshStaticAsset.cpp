#include "PrecompiledHeader.h"
#include <filesystem>
#include <PKAssets/PKAssetLoader.h>
#include "Math/FunctionsIntersect.h"
#include "Math/FunctionsMisc.h"
#include "Core/CLI/Log.h"
#include "Graphics/MeshStaticCollection.h"
#include "MeshStaticAsset.h"

namespace PK::Graphics
{
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Utilities;
    using namespace PK::Graphics;
    using namespace PK::Graphics::RHI;

    MeshStaticAsset::MeshStaticAsset() { }
    MeshStaticAsset::MeshStaticAsset(MeshStaticCollection* baseMesh, MeshStaticAllocationData* allocData) { m_staticMesh = baseMesh->Allocate(allocData); }
    MeshStaticAsset::~MeshStaticAsset() { m_staticMesh->baseMesh->Deallocate(m_staticMesh); }

    void MeshStaticAsset::AssetImport(const char* filepath, MeshStaticCollection*& baseMesh)
    {
        PK::Assets::PKAsset asset;

        PK_THROW_ASSERT(PK::Assets::OpenAsset(filepath, &asset) == 0, "Failed to open asset at path: %s", filepath);
        PK_THROW_ASSERT(asset.header->type == PK::Assets::PKAssetType::Mesh, "Trying to read a mesh from a non mesh file!")

        auto mesh = PK::Assets::ReadAsMesh(&asset);
        auto base = asset.rawData;

        PK_THROW_ASSERT(mesh->vertexAttributeCount > 0, "Trying to read a mesh with 0 vertex attributes!");
        PK_THROW_ASSERT(mesh->vertexCount > 0, "Trying to read a shader with 0 vertices!");
        PK_THROW_ASSERT(mesh->indexCount > 0, "Trying to read a shader with 0 indices!");
        PK_THROW_ASSERT(mesh->submeshCount > 0, "Trying to read a shader with 0 submeshes!");

        auto pAttributes = mesh->vertexAttributes.Get(base);
        auto pVertices = mesh->vertexBuffer.Get(base);
        auto pIndices = mesh->indexBuffer.Get(base);
        auto pSubmeshes = mesh->submeshes.Get(base);

        std::vector<MeshStaticAllocationData::SubMesh> submeshes;
        submeshes.reserve(mesh->submeshCount);

        for (auto i = 0u; i < mesh->submeshCount; ++i)
        {
            auto bounds = BoundingBox::MinMax(Functions::ToFloat3(pSubmeshes[i].bbmin), Functions::ToFloat3(pSubmeshes[i].bbmax));
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

            MeshStaticAllocationData alloc{};
            alloc.name = std::filesystem::path(GetFileName()).stem().string();

            alloc.regular.pVertices = pVertices;
            alloc.regular.pIndices = pIndices;
            alloc.regular.streamLayout = streamLayout;
            alloc.regular.pSubmeshes = submeshes.data();
            alloc.regular.indexType = mesh->indexType;
            alloc.regular.vertexCount = mesh->vertexCount;
            alloc.regular.indexCount = mesh->indexCount;
            alloc.regular.submeshCount = mesh->submeshCount;

            auto meshletMesh = mesh->meshletMesh.Get(base);
            alloc.meshlet.pSubmeshes = meshletMesh->submeshes.Get(base);
            alloc.meshlet.submeshCount = meshletMesh->submeshCount;
            alloc.meshlet.pMeshlets = meshletMesh->meshlets.Get(base);
            alloc.meshlet.meshletCount = meshletMesh->meshletCount;
            alloc.meshlet.pVertices = meshletMesh->vertices.Get(base);
            alloc.meshlet.vertexCount = meshletMesh->vertexCount;
            alloc.meshlet.pIndices = meshletMesh->indices.Get(base);
            alloc.meshlet.triangleCount = meshletMesh->triangleCount;

            m_staticMesh = baseMesh->Allocate(&alloc);
        }

        PK::Assets::CloseAsset(&asset);
    }

    const SubMeshStatic* MeshStaticAsset::GetStaticSubmesh(uint32_t localIndex) const { return m_staticMesh->GetSubmesh(localIndex); }
    uint32_t MeshStaticAsset::GetSubmeshCount() const { return m_staticMesh->submeshCount; }
}

template<>
PK::Utilities::Ref<PK::Graphics::MeshStaticAsset> PK::Core::Assets::Asset::Create()
{
    return PK::Utilities::CreateRef<PK::Graphics::MeshStaticAsset>();
}

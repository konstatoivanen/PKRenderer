#include "PrecompiledHeader.h"
#include <PKAssets/PKAssetLoader.h>
#include "Math/FunctionsIntersect.h"
#include "Math/FunctionsMisc.h"
#include "Core/Services/StringHashID.h"
#include "VirtualMesh.h"


namespace PK::Rendering::Objects
{
    using namespace PK::Math;
    using namespace PK::Core;
    using namespace PK::Core::Services;
    using namespace PK::Utilities;
    using namespace PK::Rendering;
    using namespace PK::Rendering::Objects;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    VirtualMesh::VirtualMesh()
    {
    }

    VirtualMesh::VirtualMesh(Ref<Mesh> baseMesh, const SubmeshRangeData* meshData, Ref<MeshletMesh> baseMeshletMesh, MeshletRangeData* meshletData)
    {
        m_baseMesh = baseMesh;
        m_submeshAllocation = m_baseMesh->AllocateSubmeshRange(meshData);
        m_baseMeshletMesh = baseMeshletMesh;
        m_meshletAllocation = m_baseMeshletMesh->Allocate(GraphicsAPI::GetQueues()->GetCommandBuffer(QueueType::Transfer), meshletData);
    }

    VirtualMesh::~VirtualMesh()
    {
        m_baseMesh->DeallocateSubmeshRange(m_submeshAllocation);
        m_baseMeshletMesh->Deallocate(m_meshletAllocation);
    }

    void VirtualMesh::Import(const char* filepath, Ref<Mesh>* baseMesh, Ref<MeshletMesh>* baseMeshletMesh)
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

        std::vector<BufferElement> bufferElements;
        std::vector<SubMesh> submeshes;
        submeshes.reserve(mesh->submeshCount);

        for (auto i = 0u; i < mesh->submeshCount; ++i)
        {
            auto bounds = BoundingBox::MinMax(Functions::ToFloat3(pSubmeshes[i].bbmin), Functions::ToFloat3(pSubmeshes[i].bbmax));
            submeshes.push_back({ 0u, mesh->vertexCount, pSubmeshes[i].firstIndex, pSubmeshes[i].indexCount, bounds });
        }

        for (auto i = 0u; i < mesh->vertexAttributeCount; ++i)
        {
            bufferElements.emplace_back(pAttributes[i].type, std::string(pAttributes[i].name), (byte)1u, (byte)pAttributes[i].stream, pAttributes[i].offset);
        }

        {
            PK_THROW_ASSERT(baseMesh, "Cannot create a virtual mesh without a base mesh!");
            m_baseMesh = *baseMesh;
            SubmeshRangeData meshData{};
            meshData.pVertices = pVertices;
            meshData.pIndices = pIndices;
            meshData.vertexLayout = BufferLayout(bufferElements, false);
            meshData.pSubmeshes = submeshes.data();
            meshData.indexType = mesh->indexType;
            meshData.vertexCount = mesh->vertexCount;
            meshData.indexCount = mesh->indexCount;
            meshData.submeshCount = mesh->submeshCount;
            m_submeshAllocation = m_baseMesh->AllocateSubmeshRange(&meshData);
        }

        {
            PK_THROW_ASSERT(baseMeshletMesh, "Cannot create a virtual mesh without a base meshlet mesh mesh!");
            m_baseMeshletMesh = *baseMeshletMesh;
            auto meshletMesh = mesh->meshletMesh.Get(base);
            MeshletRangeData meshletRangeData{};
            meshletRangeData.pSubmeshes = meshletMesh->submeshes.Get(base);
            meshletRangeData.submeshCount = meshletMesh->submeshCount;
            meshletRangeData.pMeshlets = meshletMesh->meshlets.Get(base);
            meshletRangeData.meshletCount = meshletMesh->meshletCount;
            meshletRangeData.pVertices = meshletMesh->vertices.Get(base);
            meshletRangeData.vertexCount = meshletMesh->vertexCount;
            meshletRangeData.pIndices = meshletMesh->indices.Get(base);
            meshletRangeData.triangleCount = meshletMesh->triangleCount;
            auto cmd = GraphicsAPI::GetQueues()->GetCommandBuffer(QueueType::Transfer);
            m_meshletAllocation = m_baseMeshletMesh->Allocate(cmd, &meshletRangeData);
        }

        PK::Assets::CloseAsset(&asset);
    }

    uint32_t VirtualMesh::GetSubmeshIndex(uint32_t submesh) const
    {
        auto idx = glm::min((uint32_t)submesh, (uint32_t)m_submeshAllocation.submeshIndices.size() - 1u);
        return m_submeshAllocation.submeshIndices.at(idx);
    }
}

template<>
PK::Utilities::Ref<PK::Rendering::Objects::VirtualMesh> PK::Core::Services::AssetImporters::Create()
{
    return PK::Utilities::CreateRef<PK::Rendering::Objects::VirtualMesh>();
}

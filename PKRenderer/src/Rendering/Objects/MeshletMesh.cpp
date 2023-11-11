#include "PrecompiledHeader.h"
#include "Utilities/VectorUtilities.h"
#include "MeshletMesh.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;
    using namespace PK::Assets::Mesh::Meshlet;

    MeshletMesh::MeshletMesh(uint32_t maxSubmeshes, uint32_t maxMeshlets, uint32_t maxVertices, uint32_t maxTriangles)
    {
        PK_THROW_ASSERT((maxTriangles * 3ull) % 4ull == 0ull, "Input triangle count x3 must be divisible by 4");

        auto flags = BufferUsage::GPUOnly | BufferUsage::TransferDst | BufferUsage::Storage | BufferUsage::Sparse;

        m_submeshBuffer = Buffer::Create(
            {
                { ElementType::Uint4, "PACKED0" },
                { ElementType::Uint4, "PACKED1" },
                { ElementType::Uint4, "PACKED2" },
            },
            maxSubmeshes, flags, "Meshlet.SubmeshBuffer");

        m_meshletBuffer = Buffer::Create(
            {
                { ElementType::Uint4, "PACKED0" },
                { ElementType::Uint4, "PACKED1" },
            },
            maxMeshlets, flags, "Meshlet.MeshletBuffer");

        m_vertexBuffer = Buffer::Create(ElementType::Uint4, maxVertices, flags, "Meshlet.VertexBuffer");
        m_indexBuffer = Buffer::Create(ElementType::Uint, (maxTriangles * 3ull) / 4ull, flags, "Meshlet.IndexBuffer");
    }

    MeshletAllocation MeshletMesh::Allocate(CommandBuffer* commandBuffer, MeshletRangeData* data)
    {
        m_submeshCount += data->submeshCount;
        m_meshletCount += data->meshletCount;
        m_vertexCount += data->vertexCount;
        m_triangleCount += data->triangleCount;

        auto submeshStride = sizeof(PKSubmesh);
        auto meshletStride = sizeof(PKMeshlet);
        auto vertexStride = sizeof(PKVertex);

        auto submeshesSize = data->submeshCount * submeshStride;
        auto meshletsSize = data->meshletCount * meshletStride;
        auto verticesSize = data->vertexCount * vertexStride;
        auto indicesSize = ((size_t)data->triangleCount * 3ull);
        PK_THROW_ASSERT((indicesSize % 4ull) == 0ull, "Index counts must be aligned to 4!");
        
        auto submeshOffset = m_submeshBuffer->SparseAllocate(submeshesSize, QueueType::Transfer);
        auto meshletOffset = m_meshletBuffer->SparseAllocate(meshletsSize, QueueType::Transfer);
        auto vertexOffset = m_vertexBuffer->SparseAllocate(verticesSize, QueueType::Transfer);
        auto indexOffset = m_indexBuffer->SparseAllocate(indicesSize, QueueType::Transfer);

        MeshletAllocation alloc{};
        alloc.firstSubmesh = (uint32_t)(submeshOffset / submeshStride);
        alloc.firstMeshlet = (uint32_t)(meshletOffset / meshletStride);
        alloc.firstVertex = (uint32_t)(vertexOffset / vertexStride);
        alloc.firstTriangle = (uint32_t)(indexOffset / 3ull);
        alloc.submeshCount = data->submeshCount;
        alloc.meshletCount = data->meshletCount;
        alloc.vertexCount = data->vertexCount;
        alloc.triangleCount = data->triangleCount;


        for (auto i = 0u; i < data->submeshCount; ++i)
        {
            data->pSubmeshes[i].firstMeshlet += alloc.firstMeshlet;
            data->pSubmeshes[i].firstVertex += alloc.firstVertex;
            data->pSubmeshes[i].firstTriangle += alloc.firstTriangle;
        }

        for (auto i = 0u; i < data->meshletCount; ++i)
        {
            data->pMeshlets[i].firstVertex += alloc.firstVertex;
            data->pMeshlets[i].firstTriangle += alloc.firstTriangle;
        }

        commandBuffer->UploadBufferSubData(m_submeshBuffer.get(), data->pSubmeshes, submeshOffset, submeshesSize);
        commandBuffer->UploadBufferSubData(m_meshletBuffer.get(), data->pMeshlets, meshletOffset, meshletsSize);
        commandBuffer->UploadBufferSubData(m_vertexBuffer.get(), data->pVertices, vertexOffset, verticesSize);
        commandBuffer->UploadBufferSubData(m_indexBuffer.get(), data->pIndices, indexOffset, indicesSize);
        m_uploadFence = commandBuffer->GetFenceRef();

        return alloc;
    }
    
    void MeshletMesh::Deallocate(const MeshletAllocation& alloc)
    {
        auto submeshStride = sizeof(PKSubmesh);
        auto meshletStride = sizeof(PKMeshlet);
        auto vertexStride = sizeof(PKVertex);

        auto submeshOffset = alloc.firstSubmesh * submeshStride;
        auto meshletOffset = alloc.firstMeshlet * meshletStride;
        auto vertexOffset = alloc.firstVertex * vertexStride;
        auto indexOffset = ((size_t)alloc.firstTriangle * 3ull);

        auto submeshesSize = alloc.submeshCount * submeshStride;
        auto meshletsSize = alloc.meshletCount * meshletStride;
        auto verticesSize = alloc.vertexCount * vertexStride;
        auto indicesSize = ((size_t)alloc.triangleCount * 3ull);

        m_submeshCount -= alloc.submeshCount;
        m_meshletCount -= alloc.meshletCount;
        m_vertexCount -= alloc.vertexCount;
        m_triangleCount -= alloc.triangleCount;

        m_submeshBuffer->SparseDeallocate({ submeshOffset, submeshesSize });
        m_meshletBuffer->SparseDeallocate({ meshletOffset, meshletsSize });
        m_vertexBuffer->SparseDeallocate({ vertexOffset, verticesSize });
        m_indexBuffer->SparseDeallocate({ indexOffset, indicesSize });
    }
}

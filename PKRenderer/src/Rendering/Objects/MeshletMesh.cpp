#include "PrecompiledHeader.h"
#include "Utilities/VectorUtilities.h"
#include "MeshletMesh.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;
    using namespace PK::Assets::Mesh::Meshlet;

    static uint32_t FindFreeOffset(const std::vector<MeshletMesh::Range>& ranges, uint32_t count)
    {
        auto first = 0u;

        for (auto& range : ranges)
        {
            if (range.first < (first + count))
            {
                first = range.first + range.count;
            }
            else
            {
                break;
            }
        }

        return first;
    }

    MeshletMesh::MeshletMesh(uint32_t maxSubmeshes, uint32_t maxMeshlets, uint32_t maxVertices, uint32_t maxIndices)
    {
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
        m_indexBuffer = Buffer::Create(ElementType::Uint, maxIndices / 4u, flags, "Meshlet.IndexBuffer");
    }

    MeshletAllocationRange MeshletMesh::Allocate(CommandBuffer* commandBuffer, 
                                                 PKSubmesh* pSubmeshes, 
                                                 uint32_t submeshCount, 
                                                 PKMeshlet* pMeshlets, 
                                                 uint32_t meshletCount, 
                                                 PKVertex* pVertices, 
                                                 uint32_t vertexCount, 
                                                 uint8_t* pIndices, 
                                                 uint32_t triangleCount)
    {
        auto alloc = GetAllocationRange(submeshCount, meshletCount, vertexCount, triangleCount);

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
        auto indicesSize = ((size_t)triangleCount * 3ull);
        PK_THROW_ASSERT((indicesSize % 4ull) == 0ull, "Index counts must be aligned to 4!");

        for (auto i = 0u; i < submeshCount; ++i)
        {
            pSubmeshes[i].firstMeshlet += alloc.firstMeshlet;
            pSubmeshes[i].firstVertex += alloc.firstVertex;
            pSubmeshes[i].firstTriangle += alloc.firstTriangle;
        }

        for (auto i = 0u; i < meshletCount; ++i)
        {
            pMeshlets[i].firstVertex += alloc.firstVertex;
            pMeshlets[i].firstTriangle += alloc.firstTriangle;
        }

        m_submeshBuffer->MakeRangeResident({ submeshOffset, submeshesSize }, QueueType::Transfer);
        m_meshletBuffer->MakeRangeResident({ meshletOffset, meshletsSize }, QueueType::Transfer);
        m_vertexBuffer->MakeRangeResident({ vertexOffset, verticesSize }, QueueType::Transfer);
        m_indexBuffer->MakeRangeResident({ indexOffset, indicesSize }, QueueType::Transfer);

        commandBuffer->UploadBufferSubData(m_submeshBuffer.get(), pSubmeshes, submeshOffset, submeshesSize);
        commandBuffer->UploadBufferSubData(m_meshletBuffer.get(), pMeshlets, meshletOffset, meshletsSize);
        commandBuffer->UploadBufferSubData(m_vertexBuffer.get(), pVertices, vertexOffset, verticesSize);
        commandBuffer->UploadBufferSubData(m_indexBuffer.get(), pIndices, indexOffset, indicesSize);

        return alloc;
    }
    
    void MeshletMesh::Deallocate(const MeshletAllocationRange& alloc)
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

        m_submeshBuffer->MakeRangeNonResident({ submeshOffset, submeshesSize });
        m_meshletBuffer->MakeRangeNonResident({ meshletOffset, meshletsSize });
        m_vertexBuffer->MakeRangeNonResident({ vertexOffset, verticesSize });
        m_indexBuffer->MakeRangeNonResident({ indexOffset, indicesSize });

        // Maybe a linked list would be better for these. :/
        for (auto i = 0u; i < m_allocationsSubmesh.size(); ++i)
        {
            if (m_allocationsSubmesh.at(i).first == alloc.firstSubmesh)
            {
                Utilities::Vector::UnorderedRemoveAt(m_allocationsSubmesh, i);
                break;
            }
        }

        for (auto i = 0u; i < m_allocationsMeshlet.size(); ++i)
        {
            if (m_allocationsMeshlet.at(i).first == alloc.firstMeshlet)
            {
                Utilities::Vector::UnorderedRemoveAt(m_allocationsMeshlet, i);
                break;
            }
        }

        for (auto i = 0u; i < m_allocationsVertex.size(); ++i)
        {
            if (m_allocationsVertex.at(i).first == alloc.firstVertex)
            {
                Utilities::Vector::UnorderedRemoveAt(m_allocationsVertex, i);
                break;
            }
        }

        for (auto i = 0u; i < m_allocationsTriangle.size(); ++i)
        {
            if (m_allocationsTriangle.at(i).first == alloc.firstVertex)
            {
                Utilities::Vector::UnorderedRemoveAt(m_allocationsTriangle, i);
                break;
            }
        }
    }
    
    void MeshletMesh::SortAllocations()
    {
        std::sort(m_allocationsSubmesh.begin(), m_allocationsSubmesh.end());
        std::sort(m_allocationsMeshlet.begin(), m_allocationsMeshlet.end());
        std::sort(m_allocationsVertex.begin(), m_allocationsVertex.end());
        std::sort(m_allocationsTriangle.begin(), m_allocationsTriangle.end());
    }

    MeshletAllocationRange MeshletMesh::GetAllocationRange(uint32_t submeshCount, uint32_t meshletCount, uint32_t vertexCount, uint32_t triangleCount)
    {
        SortAllocations();
        MeshletAllocationRange alloc{};
        alloc.firstSubmesh = FindFreeOffset(m_allocationsSubmesh, submeshCount);
        alloc.firstMeshlet = FindFreeOffset(m_allocationsMeshlet, meshletCount);
        alloc.firstVertex = FindFreeOffset(m_allocationsVertex, vertexCount);
        alloc.firstTriangle = FindFreeOffset(m_allocationsTriangle, triangleCount);
        alloc.submeshCount = submeshCount;
        alloc.meshletCount = meshletCount;
        alloc.vertexCount = vertexCount;
        alloc.triangleCount = triangleCount;
        m_allocationsSubmesh.push_back({ alloc.firstSubmesh, submeshCount });
        m_allocationsMeshlet.push_back({ alloc.firstMeshlet, meshletCount });
        m_allocationsVertex.push_back({ alloc.firstVertex, vertexCount });
        m_allocationsTriangle.push_back({ alloc.firstTriangle, triangleCount });
        return alloc;
    }
}

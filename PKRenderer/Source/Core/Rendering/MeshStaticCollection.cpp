#include "PrecompiledHeader.h"
#include "Core/Math/FunctionsMisc.h"
#include "Core/CLI/Log.h"
#include "Core/RHI/RHInterfaces.h"
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/MeshUtilities.h"
#include "MeshStaticCollection.h"

namespace PK
{
    MeshStaticCollection::MeshStaticCollection()
    {
        const uint32_t maxSubmeshes = 65535u;
        const uint32_t maxMeshlets = 65535u * 4u;
        const uint32_t maxVertices = 65535u * 32u;
        const uint32_t maxTriangles = 65535u * 16u * 3u;
        const auto flags = BufferUsage::GPUOnly | BufferUsage::TransferDst | BufferUsage::Storage | BufferUsage::Sparse;

        PK_THROW_ASSERT((maxTriangles * 3ull) % 4ull == 0ull, "Input triangle count x3 must be divisible by 4");

        m_streamLayout = VertexStreamLayout(
            {
                { ElementType::Float3, PK_RHI_VS_NORMAL, 0 },
                { ElementType::Float4, PK_RHI_VS_TANGENT, 0 },
                { ElementType::Float2, PK_RHI_VS_TEXCOORD0, 0 },
                { ElementType::Float3, PK_RHI_VS_POSITION, 1 },
            });

        m_positionsBuffer = RHI::CreateBuffer(m_streamLayout.GetStride(1u) * 2000000u, BufferUsage::SparseVertex | BufferUsage::Storage, "MeshStaticCollection.VertexPositions");
        m_attributesBuffer = RHI::CreateBuffer(m_streamLayout.GetStride(0u) * 2000000u, BufferUsage::SparseVertex, "MeshStaticCollection.VertexAttributes");
        m_indexBuffer = RHI::CreateBuffer(RHIEnumConvert::Size(m_indexType) * 2000000u, BufferUsage::SparseIndex | BufferUsage::Storage, "MeshStaticCollection.IndexBuffer");
        m_submeshBuffer = RHI::CreateBuffer<PKAssets::PKMeshletSubmesh>(maxSubmeshes, flags, "Meshlet.SubmeshBuffer");
        m_meshletBuffer = RHI::CreateBuffer<PKAssets::PKMeshlet>(maxMeshlets, flags, "Meshlet.MeshletBuffer");
        m_meshletVertexBuffer = RHI::CreateBuffer<uint4>(maxVertices, flags, "Meshlet.VertexBuffer");
        m_meshletIndexBuffer = RHI::CreateBuffer<uint32_t>((maxTriangles * 3ull) / 4ull, flags, "Meshlet.IndexBuffer");
    }

    RHIBuffer* MeshStaticCollection::GetPositionBuffer() const { return m_positionsBuffer.get(); }
    RHIBuffer* MeshStaticCollection::GetAttributeBuffer() const { return m_attributesBuffer.get(); }
    RHIBuffer* MeshStaticCollection::GetIndexBuffer() const { return m_indexBuffer.get(); }
    RHIBuffer* MeshStaticCollection::GetMeshletVertexBuffer() const { return m_meshletVertexBuffer.get(); }
    RHIBuffer* MeshStaticCollection::GetMeshletIndexBuffer() const { return m_meshletIndexBuffer.get(); }
    RHIBuffer* MeshStaticCollection::GetMeshletSubmeshBuffer() const { return m_submeshBuffer.get(); }
    RHIBuffer* MeshStaticCollection::GetMeshletBuffer() const { return m_meshletBuffer.get(); }
    const SubMeshStatic* MeshStaticCollection::GetSubmesh(uint32_t index) const { return m_staticSubmeshes[index]; }
    bool MeshStaticCollection::HasPendingUpload() const { return !m_uploadFence.WaitInvalidate(0ull); }


    MeshStatic* MeshStaticCollection::Allocate(MeshStaticDescriptor* desc)
    {
        PK_LOG_VERBOSE_FUNC("sm:%u, ml:%u, mlvc:%u, mltc:%u, vc:%u, tc:%u",
            desc->meshlet.submeshCount,
            desc->meshlet.meshletCount,
            desc->meshlet.vertexCount,
            desc->meshlet.triangleCount,
            desc->regular.vertexCount,
            desc->regular.indexCount);

        PK_THROW_ASSERT(desc->meshlet.submeshCount == desc->regular.submeshCount, "Submesh count missmatch");

        MeshStatic* staticMesh = nullptr;

        // Prefer to allocate at last deallocation index.
        // This is a bit of a hack for assets to trivially reuse the same index on reimport.
        staticMesh = m_staticMeshes.NewAt(m_preferredIndex);
        staticMesh->baseMesh = this;
        m_preferredIndex = -1;

        m_submeshCount += desc->meshlet.submeshCount;
        m_meshletCount += desc->meshlet.meshletCount;
        m_meshletVertexCount += desc->meshlet.vertexCount;
        m_meshletVertexCount += desc->meshlet.triangleCount;
        m_vertexCount += desc->regular.vertexCount;
        m_indexCount += desc->regular.indexCount;

        auto submeshStride = sizeof(PKAssets::PKMeshletSubmesh);
        auto meshletStride = sizeof(PKAssets::PKMeshlet);
        auto meshletVertexStride = sizeof(PKAssets::PKMeshletVertex);
        auto positionsStride = m_streamLayout.GetStride(1u);
        auto attributesStride = m_streamLayout.GetStride(0u);
        auto indexStride = RHIEnumConvert::Size(m_indexType);

        auto submeshesSize = desc->meshlet.submeshCount * submeshStride;
        auto meshletsSize = desc->meshlet.meshletCount * meshletStride;
        auto meshletVerticesSize = desc->meshlet.vertexCount * meshletVertexStride;
        auto meshletIndicesSize = ((size_t)desc->meshlet.triangleCount * 3ull);
        auto positionsSize = desc->regular.vertexCount * positionsStride;
        auto attributesSize = desc->regular.vertexCount * attributesStride;
        auto indicesSize = desc->regular.indexCount * indexStride;

        PK_THROW_ASSERT((meshletIndicesSize % 4ull) == 0ull, "Index counts must be aligned to 4!");

        auto submeshOffset = m_submeshBuffer->SparseAllocate(submeshesSize, QueueType::Transfer);
        auto meshletOffset = m_meshletBuffer->SparseAllocate(meshletsSize, QueueType::Transfer);
        auto meshletVertexOffset = m_meshletVertexBuffer->SparseAllocate(meshletVerticesSize, QueueType::Transfer);
        auto meshletIndexOffset = m_meshletIndexBuffer->SparseAllocate(meshletIndicesSize, QueueType::Transfer);
        auto positionsOffset = m_positionsBuffer->SparseAllocate(positionsSize, QueueType::Transfer);
        auto attributesOffset = m_attributesBuffer->SparseAllocate(attributesSize, QueueType::Transfer);
        auto indexOffset = m_indexBuffer->SparseAllocate(indicesSize, QueueType::Transfer);

        PK_THROW_ASSERT((meshletIndexOffset % 12ull) == 0ull, "Meshlet Index offsets must be aligned to 12!");

        // Used accross mesh types
        // Leverage submesh buffer offset by using it on cpu side submesh index as well.
        staticMesh->submeshFirst = (uint32_t)(submeshOffset / submeshStride);
        staticMesh->submeshCount = desc->meshlet.submeshCount;

        staticMesh->meshletFirst = (uint32_t)(meshletOffset / meshletStride);
        staticMesh->meshletCount = desc->meshlet.meshletCount;
        staticMesh->meshletVertexFirst = (uint32_t)(meshletVertexOffset / meshletVertexStride);
        staticMesh->meshletVertexCount = desc->meshlet.vertexCount;
        staticMesh->meshletTriangleFirst = (uint32_t)(meshletIndexOffset / 3ull);
        staticMesh->meshletTriangleCount = desc->meshlet.triangleCount;

        staticMesh->vertexFirst = (uint32_t)(positionsOffset / positionsStride);
        staticMesh->vertexCount = (uint32_t)(positionsSize / positionsStride);
        staticMesh->indexFirst = (uint32_t)(indexOffset / indexStride);
        staticMesh->indexCount = (uint32_t)(indicesSize / indexStride);
        staticMesh->name = desc->name;

        for (auto i = 0u; i < staticMesh->submeshCount; ++i)
        {
            desc->meshlet.pSubmeshes[i].firstMeshlet += staticMesh->meshletFirst;

            auto submesh = m_staticSubmeshes.NewAt(staticMesh->submeshFirst + i);
            submesh->meshletFirst = desc->meshlet.pSubmeshes[i].firstMeshlet;
            submesh->meshletCount = desc->meshlet.pSubmeshes[i].meshletCount;
            submesh->vertexFirst = desc->regular.pSubmeshes[i].vertexFirst + staticMesh->vertexFirst;
            submesh->vertexCount = desc->regular.pSubmeshes[i].vertexCount;
            submesh->indexFirst = desc->regular.pSubmeshes[i].indexFirst + staticMesh->indexFirst;
            submesh->indexCount = desc->regular.pSubmeshes[i].indexCount;
            submesh->bounds = desc->regular.pSubmeshes[i].bounds;
            submesh->name = FixedString128("%s.Submesh%u", desc->name.c_str(), i).c_str();
        }

        for (auto i = 0u; i < staticMesh->meshletCount; ++i)
        {
            desc->meshlet.pMeshlets[i].vertexFirst += staticMesh->meshletVertexFirst;
            desc->meshlet.pMeshlets[i].triangleFirst += staticMesh->meshletTriangleFirst;
        }

        auto commandBuffer = CommandBufferExt(RHI::GetCommandBuffer(QueueType::Transfer));
        commandBuffer.UploadBufferSubData(m_submeshBuffer.get(), desc->meshlet.pSubmeshes, submeshOffset, submeshesSize);
        commandBuffer.UploadBufferSubData(m_meshletBuffer.get(), desc->meshlet.pMeshlets, meshletOffset, meshletsSize);
        commandBuffer.UploadBufferSubData(m_meshletVertexBuffer.get(), desc->meshlet.pVertices, meshletVertexOffset, meshletVerticesSize);
        commandBuffer.UploadBufferSubData(m_meshletIndexBuffer.get(), desc->meshlet.pIndices, meshletIndexOffset, meshletIndicesSize);

        // Rewrite indices if using a different index format
        if (RHIEnumConvert::Size(desc->regular.indexType) == 2u && indexStride == 4u)
        {
            auto view = commandBuffer.BeginBufferWrite<uint32_t>(m_indexBuffer.get(), staticMesh->indexFirst, desc->regular.indexCount);
            Math::ReinterpretIndex16ToIndex32(view.data, reinterpret_cast<uint16_t*>(desc->regular.pIndices), desc->regular.indexCount);
            commandBuffer->EndBufferWrite(m_indexBuffer.get());
        }
        else
        {
            commandBuffer.UploadBufferSubData(m_indexBuffer.get(), desc->regular.pIndices, indexOffset, indicesSize);
        }

        // Align vertices into split layout if necessary
        MeshUtilities::AlignVertexStreams((char*)desc->regular.pVertices, desc->regular.vertexCount, desc->regular.streamLayout, m_streamLayout);

        commandBuffer.UploadBufferSubData(m_attributesBuffer.get(), (char*)desc->regular.pVertices, attributesOffset, attributesSize);
        commandBuffer.UploadBufferSubData(m_positionsBuffer.get(), (char*)desc->regular.pVertices + attributesSize, positionsOffset, positionsSize);

        m_uploadFence = commandBuffer->GetFenceRef();

        return staticMesh;
    }

    void MeshStaticCollection::Deallocate(MeshStatic* mesh)
    {
        auto submeshStride = sizeof(PKAssets::PKMeshletSubmesh);
        auto meshletStride = sizeof(PKAssets::PKMeshlet);
        auto meshletVertexStride = sizeof(PKAssets::PKMeshletVertex);
        auto positionsStride = m_streamLayout.GetStride(1u);
        auto attributesStride = m_streamLayout.GetStride(0u);
        auto indexStride = RHIEnumConvert::Size(m_indexType);

        auto submeshOffset = mesh->submeshFirst * submeshStride;
        auto meshletOffset = mesh->meshletFirst * meshletStride;
        auto meshletVertexOffset = mesh->meshletVertexFirst * meshletVertexStride;
        auto meshletIndexOffset = ((size_t)mesh->meshletTriangleFirst * 3ull);
        auto positionsOffset = mesh->vertexFirst * positionsStride;
        auto attributesOffset = mesh->vertexFirst * attributesStride;
        auto indexOffset = mesh->indexFirst * indexStride;

        auto submeshesSize = mesh->submeshCount * submeshStride;
        auto meshletsSize = mesh->meshletCount * meshletStride;
        auto meshletVerticesSize = mesh->meshletVertexCount * meshletVertexStride;
        auto meshletIndicesSize = ((size_t)mesh->meshletTriangleCount * 3ull);
        auto positionsSize = mesh->vertexCount * positionsStride;
        auto attributesSize = mesh->vertexCount * attributesStride;
        auto indicesSize = mesh->indexCount * indexStride;

        m_submeshBuffer->SparseDeallocate({ submeshOffset, submeshesSize });
        m_meshletBuffer->SparseDeallocate({ meshletOffset, meshletsSize });
        m_meshletVertexBuffer->SparseDeallocate({ meshletVertexOffset, meshletVerticesSize });
        m_meshletIndexBuffer->SparseDeallocate({ meshletIndexOffset, meshletIndicesSize });
        m_positionsBuffer->SparseDeallocate({ positionsOffset, positionsSize });
        m_attributesBuffer->SparseDeallocate({ attributesOffset, attributesSize });
        m_indexBuffer->SparseDeallocate({ indexOffset, indicesSize });

        m_submeshCount -= mesh->submeshCount;
        m_meshletCount -= mesh->meshletCount;
        m_meshletVertexCount -= mesh->meshletVertexCount;
        m_meshletTriangleCount -= mesh->meshletTriangleCount;
        m_vertexCount -= mesh->vertexCount;
        m_indexCount -= mesh->indexCount;
        m_preferredIndex = m_staticMeshes.GetIndex(mesh);

        for (auto i = 0u; i < mesh->submeshCount; ++i)
        {
            auto submeshIndex = mesh->submeshFirst + i;
            m_staticSubmeshes.Delete(submeshIndex);
        }

        m_staticMeshes.Delete(mesh);
    }

    bool MeshStaticCollection::TryGetAccelerationStructureGeometryInfo(uint32_t globalSubmeshIndex, AccelerationStructureGeometryInfo* outInfo) const
    {
        if (!HasPendingUpload())
        {
            const auto& sm = GetSubmesh(globalSubmeshIndex);
            outInfo->name = sm->name;
            outInfo->vertexBuffer = m_positionsBuffer.get();
            outInfo->indexBuffer = m_indexBuffer.get();
            outInfo->vertexOffset = 0u;
            outInfo->vertexStride = m_streamLayout.GetStride(1u);
            outInfo->vertexFirst = sm->vertexFirst;
            outInfo->vertexCount = sm->vertexCount;
            outInfo->indexStride = RHIEnumConvert::Size(m_indexType);
            outInfo->indexFirst = sm->indexFirst;
            outInfo->indexCount = sm->indexCount;
            outInfo->customIndex = 0u;
            return true;
        }

        return false;
    }

    const SubMeshStatic* MeshStatic::GetSubmesh(uint32_t localIndex) const
    {
        return baseMesh->GetSubmesh(GetGlobalSubmeshIndex(localIndex));
    }

    bool MeshStatic::TryGetAccelerationStructureGeometryInfo(uint32_t localIndex, AccelerationStructureGeometryInfo* outInfo) const
    {
        return baseMesh->TryGetAccelerationStructureGeometryInfo(GetGlobalSubmeshIndex(localIndex), outInfo);
    }
}

#include "PrecompiledHeader.h"
#include "Math/FunctionsMisc.h"
#include "Rendering/HashCache.h"
#include "StaticSceneMesh.h"

namespace PK::Rendering::Objects
{
    using namespace PK::Math;
    using namespace PK::Utilities;
    using namespace PK::Core::Services;
    using namespace PK::Rendering::RHI;
    using namespace PK::Rendering::RHI::Objects;

    StaticSceneMesh::StaticSceneMesh()
    {
        const uint32_t maxSubmeshes = 65535u;
        const uint32_t maxMeshlets = 65535u * 4u;
        const uint32_t maxVertices = 65535u * 32u;
        const uint32_t maxTriangles = 65535u * 16u * 3u;
        const auto flags = BufferUsage::GPUOnly | BufferUsage::TransferDst | BufferUsage::Storage | BufferUsage::Sparse;

        PK_THROW_ASSERT((maxTriangles * 3ull) % 4ull == 0ull, "Input triangle count x3 must be divisible by 4");

        m_positionsBuffer = Buffer::Create({ { ElementType::Float3, PK_VS_POSITION } }, 2000000, BufferUsage::SparseVertex | BufferUsage::Storage, "StaticSceneMesh.VertexPositions");
        
        m_attributesBuffer = Buffer::Create(
            {
                { ElementType::Float3, PK_VS_NORMAL },
                { ElementType::Float4, PK_VS_TANGENT },
                { ElementType::Float2, PK_VS_TEXCOORD0 },
            },
            2000000, BufferUsage::SparseVertex, "StaticSceneMesh.VertexAttributes");

        m_indexBuffer = Buffer::Create(ElementType::Uint, 2000000, BufferUsage::SparseIndex | BufferUsage::Storage, "StaticSceneMesh.IndexBuffer");

        m_submeshBuffer = Buffer::Create(
            {
                { ElementType::Uint4, "PACKED0" },
                { ElementType::Uint4, "PACKED1" },
            },
            maxSubmeshes, flags, "Meshlet.SubmeshBuffer");

        m_meshletBuffer = Buffer::Create(
            {
                { ElementType::Uint4, "PACKED0" },
                { ElementType::Uint4, "PACKED1" },
            },
            maxMeshlets, flags, "Meshlet.MeshletBuffer");

        m_meshletVertexBuffer = Buffer::Create(ElementType::Uint4, maxVertices, flags, "Meshlet.VertexBuffer");
        m_meshletIndexBuffer = Buffer::Create(ElementType::Uint, (maxTriangles * 3ull) / 4ull, flags, "Meshlet.IndexBuffer");
        AssignMeshletBuffers();
    }

    StaticMesh* StaticSceneMesh::Allocate(StaticMeshAllocationData* data)
    {
        PK_LOG_VERBOSE("Static Mesh Allocation");
        PK_LOG_SCOPE_INDENT(meshlet);
        
        PK_THROW_ASSERT(data->meshlet.submeshCount == data->regular.submeshCount, "Submesh count missmatch");
        
        auto staticMesh = m_staticMeshes.New();
        staticMesh->baseMesh = this;

        m_submeshCount += data->meshlet.submeshCount;
        m_meshletCount += data->meshlet.meshletCount;
        m_meshletVertexCount += data->meshlet.vertexCount;
        m_meshletVertexCount += data->meshlet.triangleCount;
        m_vertexCount += data->regular.vertexCount;
        m_indexCount += data->regular.indexCount;

        auto submeshStride = sizeof(PK::Assets::Mesh::Meshlet::PKSubmesh);
        auto meshletStride = sizeof(PK::Assets::Mesh::Meshlet::PKMeshlet);
        auto meshletVertexStride = sizeof(PK::Assets::Mesh::Meshlet::PKVertex);
        auto positionsStride = m_positionsBuffer->GetLayout().GetStride();
        auto attributesStride = m_attributesBuffer->GetLayout().GetStride();
        auto indexStride = m_indexBuffer->GetLayout().GetStride();

        auto submeshesSize = data->meshlet.submeshCount * submeshStride;
        auto meshletsSize = data->meshlet.meshletCount * meshletStride;
        auto meshletVerticesSize = data->meshlet.vertexCount * meshletVertexStride;
        auto meshletIndicesSize = ((size_t)data->meshlet.triangleCount * 3ull);
        auto positionsSize = data->regular.vertexCount * positionsStride;
        auto attributesSize = data->regular.vertexCount * attributesStride;
        auto indicesSize = data->regular.indexCount * indexStride;

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
        staticMesh->submeshCount = data->meshlet.submeshCount;

        staticMesh->meshletFirst = (uint32_t)(meshletOffset / meshletStride);
        staticMesh->meshletCount = data->meshlet.meshletCount;
        staticMesh->meshletVertexFirst = (uint32_t)(meshletVertexOffset / meshletVertexStride);
        staticMesh->meshletVertexCount = data->meshlet.vertexCount;
        staticMesh->meshletTriangleFirst = (uint32_t)(meshletIndexOffset / 3ull);
        staticMesh->meshletTriangleCount = data->meshlet.triangleCount;

        staticMesh->vertexFirst = (uint32_t)(positionsOffset / positionsStride);
        staticMesh->vertexCount = (uint32_t)(positionsSize / positionsStride);
        staticMesh->indexFirst = (uint32_t)(indexOffset / indexStride);
        staticMesh->indexCount = (uint32_t)(indicesSize / indexStride);
        
        staticMesh->nameHashId = data->nameHashId;

        for (auto i = 0u; i < staticMesh->submeshCount; ++i)
        {
            data->meshlet.pSubmeshes[i].firstMeshlet += staticMesh->meshletFirst;

            auto submesh = m_staticSubmeshes.NewAt(staticMesh->submeshFirst + i);
            submesh->meshletFirst = data->meshlet.pSubmeshes[i].firstMeshlet;
            submesh->meshletCount = data->meshlet.pSubmeshes[i].meshletCount;
            submesh->vertexFirst = data->regular.pSubmeshes[i].vertexFirst + staticMesh->vertexFirst;
            submesh->vertexCount = data->regular.pSubmeshes[i].vertexCount;
            submesh->indexFirst = data->regular.pSubmeshes[i].indexFirst + staticMesh->indexFirst;
            submesh->indexCount = data->regular.pSubmeshes[i].indexCount;
            submesh->bounds = data->regular.pSubmeshes[i].bounds;

            auto name = StringHashID::IDToString(data->nameHashId) + std::string(".Submesh") + std::to_string(i);
            submesh->nameHashId = StringHashID::StringToID(name.c_str());
        }

        for (auto i = 0u; i < staticMesh->meshletCount; ++i)
        {
            data->meshlet.pMeshlets[i].vertexFirst += staticMesh->meshletVertexFirst;
            data->meshlet.pMeshlets[i].triangleFirst += staticMesh->meshletTriangleFirst;
        }

        auto commandBuffer = GraphicsAPI::GetQueues()->GetCommandBuffer(QueueType::Transfer);
        commandBuffer->UploadBufferSubData(m_submeshBuffer.get(), data->meshlet.pSubmeshes, submeshOffset, submeshesSize);
        commandBuffer->UploadBufferSubData(m_meshletBuffer.get(), data->meshlet.pMeshlets, meshletOffset, meshletsSize);
        commandBuffer->UploadBufferSubData(m_meshletVertexBuffer.get(), data->meshlet.pVertices, meshletVertexOffset, meshletVerticesSize);
        commandBuffer->UploadBufferSubData(m_meshletIndexBuffer.get(), data->meshlet.pIndices, meshletIndexOffset, meshletIndicesSize);
        
        // Rewrite indices if using a different index format
        if (ElementConvert::Size(data->regular.indexType) == 2 && indexStride == 4u)
        {
            auto view = commandBuffer->BeginBufferWrite<uint32_t>(m_indexBuffer.get(), staticMesh->indexFirst, data->regular.indexCount);
            Functions::ReinterpretIndex16ToIndex32(view.data, reinterpret_cast<uint16_t*>(data->regular.pIndices), data->regular.indexCount);
            commandBuffer->EndBufferWrite(m_indexBuffer.get());
        }
        else
        {
            commandBuffer->UploadBufferSubData(m_indexBuffer.get(), data->regular.pIndices, indexOffset, indicesSize);
        }
            
        // Align vertices into split layout if necessary
        AlignVertices((char*)data->regular.pVertices, data->regular.vertexCount, data->regular.vertexLayout);

        commandBuffer->UploadBufferSubData(m_attributesBuffer.get(), (char*)data->regular.pVertices, attributesOffset, attributesSize);
        commandBuffer->UploadBufferSubData(m_positionsBuffer.get(), (char*)data->regular.pVertices + attributesSize, positionsOffset, positionsSize);

        m_uploadFence = commandBuffer->GetFenceRef();

        return staticMesh;
    }

    void StaticSceneMesh::Deallocate(StaticMesh* mesh)
    {
        auto submeshStride = sizeof(PK::Assets::Mesh::Meshlet::PKSubmesh);
        auto meshletStride = sizeof(PK::Assets::Mesh::Meshlet::PKMeshlet);
        auto meshletVertexStride = sizeof(PK::Assets::Mesh::Meshlet::PKVertex);
        auto positionsStride = m_positionsBuffer->GetLayout().GetStride();
        auto attributesStride = m_attributesBuffer->GetLayout().GetStride();
        auto indexStride = m_indexBuffer->GetLayout().GetStride();

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

        for (auto i = 0u; i < mesh->submeshCount; ++i)
        {
            auto submeshIndex = mesh->submeshFirst + i;
            m_staticSubmeshes.Delete(submeshIndex);
        }

        m_staticMeshes.Delete(mesh);
    }

    void StaticSceneMesh::AssignMeshletBuffers() const
    {
        auto hash = HashCache::Get();
        GraphicsAPI::SetBuffer(hash->pk_Meshlet_Submeshes, m_submeshBuffer.get());
        GraphicsAPI::SetBuffer(hash->pk_Meshlets, m_meshletBuffer.get());
        GraphicsAPI::SetBuffer(hash->pk_Meshlet_Vertices, m_meshletVertexBuffer.get());
        GraphicsAPI::SetBuffer(hash->pk_Meshlet_Indices, m_meshletIndexBuffer.get());
    }

    bool StaticSceneMesh::TryGetAccelerationStructureGeometryInfo(uint32_t globalSubmeshIndex, RHI::Objects::AccelerationStructureGeometryInfo* outInfo) const
    {
        if (!HasPendingUpload())
        {
            const auto& sm = GetSubmesh(globalSubmeshIndex);
            outInfo->vertexBuffer = m_positionsBuffer.get();
            outInfo->indexBuffer = m_indexBuffer.get();
            outInfo->vertexOffset = 0u;
            outInfo->firstVertex = sm->vertexFirst;
            outInfo->vertexCount = sm->vertexCount;
            outInfo->firstIndex = sm->indexFirst;
            outInfo->indexCount = sm->indexCount;
            outInfo->customIndex = 0u;
            outInfo->nameHashId = sm->nameHashId; 
            return true;
        }

        return false;
    }

    void StaticSceneMesh::AlignVertices(char* vertices, size_t vcount, const RHI::BufferLayout& layout)
    {
        auto stride = layout.GetStride();
        auto fullStride = 0ull;
        auto needsAlignment = false;
        auto streamIndex = 0u;

        auto vertexBuffers = { m_attributesBuffer.get(), m_positionsBuffer.get() };

        for (auto& vertexBuffer : vertexBuffers)
        {
            const auto& targetLayout = vertexBuffer->GetLayout();

            for (auto& element : targetLayout)
            {
                uint32_t elementIndex = 0u;
                auto other = layout.TryGetElement(element.NameHashId, &elementIndex);
                PK_THROW_ASSERT(other, "Required element not present in source layout!");
                PK_THROW_ASSERT(other->Type == element.Type, "Element type missmatch!");
                needsAlignment |= other->Location != streamIndex || other->Offset != element.Offset;
            }

            fullStride += targetLayout.GetStride();
            ++streamIndex;
        }

        PK_THROW_ASSERT(fullStride == stride, "Layout stride missmatch!");

        if (!needsAlignment)
        {
            return;
        }

        auto buffer = (char*)calloc(vcount, stride);
        auto subBuffer = buffer;

        for (auto& vertexBuffer : vertexBuffers)
        {
            const auto& targetLayout = vertexBuffer->GetLayout();
            auto targetStride = targetLayout.GetStride();

            for (auto& targetElement : targetLayout)
            {
                uint32_t elementIndex = 0u;
                auto element = layout.TryGetElement(targetElement.NameHashId, &elementIndex);
                auto srcOffset = element->Offset;
                auto size = targetElement.Size();

                for (auto i = 0u; i < vcount; ++i)
                {
                    memcpy(subBuffer + targetStride * i + targetElement.Offset, vertices + stride * i + srcOffset, size);
                }
            }

            subBuffer += targetStride * vcount;
        }

        memcpy(vertices, buffer, vcount * stride);
        free(buffer);
    }

    const StaticSubMesh* StaticMesh::GetSubmesh(uint32_t localIndex) const
    {
        return baseMesh->GetSubmesh(GetGlobalSubmeshIndex(localIndex));
    }
   
    bool StaticMesh::TryGetAccelerationStructureGeometryInfo(uint32_t localIndex, RHI::Objects::AccelerationStructureGeometryInfo* outInfo) const
    {
        return baseMesh->TryGetAccelerationStructureGeometryInfo(GetGlobalSubmeshIndex(localIndex), outInfo);
    }
}
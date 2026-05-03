#include "PrecompiledHeader.h"
#include <PKAssets/PKAssetLoader.h>
#include "Core/Rendering/CommandBufferExt.h"
#include "Core/Rendering/MeshUtilities.h"
#include "Core/Math/Extended.h"
#include "Core/CLI/Log.h"
#include "Mesh.h"

namespace PK
{
    IMesh::~IMesh() = default;
    IMeshlets::~IMeshlets() = default;
    IRayTracingGeometry::~IRayTracingGeometry() = default;


    MeshStaticAllocator::MeshStaticAllocator()
    {
        const uint32_t maxSubmeshes = 65535u;
        const uint32_t maxMeshlets = 65535u * 4u;
        const uint32_t maxVertices = 65535u * 32u;
        const uint32_t maxTriangles = 65535u * 16u * 3u;
        const auto flags = BufferUsage::GPUOnly | BufferUsage::TransferDst | BufferUsage::Storage | BufferUsage::Sparse;

        PK_FATAL_ASSERT((maxTriangles * 3ull) % 4ull == 0ull, "Input triangle count x3 must be divisible by 4");

        m_streamLayout = VertexStreamLayout(
        {
            { ElementType::Half4, PK_RHI_VS_NORMAL, 0 },
            { ElementType::Half4, PK_RHI_VS_TANGENT, 0 },
            { ElementType::Half2, PK_RHI_VS_TEXCOORD0, 0 },
            { ElementType::Float3, PK_RHI_VS_POSITION, 1 },
        });

        m_vertexBuffers.ClearFast();
        m_vertexBuffers.Add(RHI::CreateBuffer(m_streamLayout.GetStride(0u) * 2000000u, BufferUsage::SparseVertex, "MeshStaticCollection.VertexAttributes"));
        m_vertexBuffers.Add(RHI::CreateBuffer(m_streamLayout.GetStride(1u) * 2000000u, BufferUsage::SparseVertex | BufferUsage::Storage, "MeshStaticCollection.VertexPositions"));
        m_indexBuffer = RHI::CreateBuffer(RHIEnumConvert::Size(m_indexType) * 2000000u, BufferUsage::SparseIndex | BufferUsage::Storage, "MeshStaticCollection.IndexBuffer");
        m_submeshBuffer = RHI::CreateBuffer<PKAssets::PKMeshletSubmesh>(maxSubmeshes, flags, "Meshlet.SubmeshBuffer");
        m_meshletBuffer = RHI::CreateBuffer<PKAssets::PKMeshlet>(maxMeshlets, flags, "Meshlet.MeshletBuffer");
        m_meshletVertexBuffer = RHI::CreateBuffer<uint4>(maxVertices, flags, "Meshlet.VertexBuffer");
        m_meshletIndexBuffer = RHI::CreateBuffer<uint32_t>((maxTriangles * 3ull) / 4ull, flags, "Meshlet.IndexBuffer");
    }

    MeshStaticAllocator::Allocation* MeshStaticAllocator::Allocate(const MeshStaticDescriptor& desc)
    {
        PK_LOG_VERBOSE_FUNC_FMT("sm:%u, ml:%u, mlvc:%u, mltc:%u, vc:%u, tc:%u",
            desc.meshlets.submeshCount,
            desc.meshlets.meshletCount,
            desc.meshlets.vertexCount,
            desc.meshlets.triangleCount,
            desc.regular.vertexCount,
            desc.regular.indexCount);

        PK_FATAL_ASSERT(desc.meshlets.submeshCount == desc.regular.submeshCount, "Submesh count missmatch");

        MeshStaticAllocator::Allocation* allocation = nullptr;

        // Prefer to allocate at last deallocation index.
        // This is a bit of a hack for assets to trivially reuse the same index on reimport.
        allocation = m_allocations.NewAt(m_preferredIndex);
        allocation->allocator = this;
        m_preferredIndex = -1;

        m_submeshCount += desc.meshlets.submeshCount;
        m_meshletCount += desc.meshlets.meshletCount;
        m_meshletVertexCount += desc.meshlets.vertexCount;
        m_meshletVertexCount += desc.meshlets.triangleCount;
        m_vertexCount += desc.regular.vertexCount;
        m_indexCount += desc.regular.indexCount;

        auto submeshStride = sizeof(PKAssets::PKMeshletSubmesh);
        auto meshletStride = sizeof(PKAssets::PKMeshlet);
        auto meshletVertexStride = sizeof(PKAssets::PKMeshletVertex);
        auto positionsStride = m_streamLayout.GetStride(1u);
        auto attributesStride = m_streamLayout.GetStride(0u);
        auto indexStride = RHIEnumConvert::Size(m_indexType);

        auto submeshesSize = desc.meshlets.submeshCount * submeshStride;
        auto meshletsSize = desc.meshlets.meshletCount * meshletStride;
        auto meshletVerticesSize = desc.meshlets.vertexCount * meshletVertexStride;
        auto meshletIndicesSize = ((size_t)desc.meshlets.triangleCount * 3ull);
        auto positionsSize = desc.regular.vertexCount * positionsStride;
        auto attributesSize = desc.regular.vertexCount * attributesStride;
        auto indicesSize = desc.regular.indexCount * indexStride;

        PK_FATAL_ASSERT((meshletIndicesSize % 4ull) == 0ull, "Index counts must be aligned to 4!");

        auto submeshOffset = m_submeshBuffer->SparseAllocate(submeshesSize, QueueType::Transfer);
        auto meshletOffset = m_meshletBuffer->SparseAllocate(meshletsSize, QueueType::Transfer);
        auto meshletVertexOffset = m_meshletVertexBuffer->SparseAllocate(meshletVerticesSize, QueueType::Transfer);
        auto meshletIndexOffset = m_meshletIndexBuffer->SparseAllocate(meshletIndicesSize, QueueType::Transfer);
        auto attributesOffset = m_vertexBuffers[0]->SparseAllocate(attributesSize, QueueType::Transfer);
        auto positionsOffset = m_vertexBuffers[1]->SparseAllocate(positionsSize, QueueType::Transfer);
        auto indexOffset = m_indexBuffer->SparseAllocate(indicesSize, QueueType::Transfer);

        PK_FATAL_ASSERT((meshletIndexOffset % 12ull) == 0ull, "Meshlet Index offsets must be aligned to 12!");

        // Used accross mesh types
        // Leverage submesh buffer offset by using it on cpu side submesh index as well.
        allocation->submeshFirst = (uint32_t)(submeshOffset / submeshStride);
        allocation->submeshCount = desc.meshlets.submeshCount;

        allocation->meshletFirst = (uint32_t)(meshletOffset / meshletStride);
        allocation->meshletCount = desc.meshlets.meshletCount;
        allocation->meshletVertexFirst = (uint32_t)(meshletVertexOffset / meshletVertexStride);
        allocation->meshletVertexCount = desc.meshlets.vertexCount;
        allocation->meshletTriangleFirst = (uint32_t)(meshletIndexOffset / 3ull);
        allocation->meshletTriangleCount = desc.meshlets.triangleCount;

        allocation->vertexFirst = (uint32_t)(positionsOffset / positionsStride);
        allocation->vertexCount = (uint32_t)(positionsSize / positionsStride);
        allocation->indexFirst = (uint32_t)(indexOffset / indexStride);
        allocation->indexCount = (uint32_t)(indicesSize / indexStride);
        allocation->name = desc.name;

        for (auto i = 0u; i < allocation->submeshCount; ++i)
        {
            desc.meshlets.pSubmeshes[i].firstMeshlet += allocation->meshletFirst;
            auto submesh = m_submeshes.NewAt(allocation->submeshFirst + i);
            submesh->meshletFirst = desc.meshlets.pSubmeshes[i].firstMeshlet;
            submesh->meshletCount = desc.meshlets.pSubmeshes[i].meshletCount;
            submesh->vertexFirst = desc.regular.pSubmeshes[i].vertexFirst + allocation->vertexFirst;
            submesh->vertexCount = desc.regular.pSubmeshes[i].vertexCount;
            submesh->indexFirst = desc.regular.pSubmeshes[i].indexFirst + allocation->indexFirst;
            submesh->indexCount = desc.regular.pSubmeshes[i].indexCount;
            submesh->bounds = desc.regular.pSubmeshes[i].bounds;
            submesh->name = FixedString128("%s.Submesh%u", desc.name.c_str(), i).c_str();
        }

        for (auto i = 0u; i < allocation->meshletCount; ++i)
        {
            desc.meshlets.pMeshlets[i].vertexFirst += allocation->meshletVertexFirst;
            desc.meshlets.pMeshlets[i].triangleFirst += allocation->meshletTriangleFirst;
        }

        auto commandBuffer = CommandBufferExt(RHI::GetCommandBuffer(QueueType::Transfer));
        commandBuffer.UploadBufferSubData(m_submeshBuffer.get(), desc.meshlets.pSubmeshes, submeshOffset, submeshesSize);
        commandBuffer.UploadBufferSubData(m_meshletBuffer.get(), desc.meshlets.pMeshlets, meshletOffset, meshletsSize);
        commandBuffer.UploadBufferSubData(m_meshletVertexBuffer.get(), desc.meshlets.pVertices, meshletVertexOffset, meshletVerticesSize);
        commandBuffer.UploadBufferSubData(m_meshletIndexBuffer.get(), desc.meshlets.pIndices, meshletIndexOffset, meshletIndicesSize);

        // Rewrite indices if using a different index format
        if (RHIEnumConvert::Size(desc.regular.indexType) == 2u && indexStride == 4u)
        {
            auto view = commandBuffer.BeginBufferWrite<uint32_t>(m_indexBuffer.get(), allocation->indexFirst, desc.regular.indexCount);
            Memory::CopyCastArray(view.data, reinterpret_cast<uint16_t*>(desc.regular.pIndices), desc.regular.indexCount);
            commandBuffer->EndBufferWrite(m_indexBuffer.get());
        }
        else
        {
            commandBuffer.UploadBufferSubData(m_indexBuffer.get(), desc.regular.pIndices, indexOffset, indicesSize);
        }

        // Align vertices into split layout if necessary
        MeshUtilities::AlignVertexStreams((char*)desc.regular.pVertices, desc.regular.vertexCount, desc.regular.streamLayout, m_streamLayout);

        commandBuffer.UploadBufferSubData(m_vertexBuffers[0].get(), (char*)desc.regular.pVertices, attributesOffset, attributesSize);
        commandBuffer.UploadBufferSubData(m_vertexBuffers[1].get(), (char*)desc.regular.pVertices + attributesSize, positionsOffset, positionsSize);

        m_uploadFence = commandBuffer->GetFenceRef();

        return allocation;
    }

    void MeshStaticAllocator::Deallocate(Allocation* allocation)
    {
        auto submeshStride = sizeof(PKAssets::PKMeshletSubmesh);
        auto meshletStride = sizeof(PKAssets::PKMeshlet);
        auto meshletVertexStride = sizeof(PKAssets::PKMeshletVertex);
        auto positionsStride = m_streamLayout.GetStride(1u);
        auto attributesStride = m_streamLayout.GetStride(0u);
        auto indexStride = RHIEnumConvert::Size(m_indexType);

        auto submeshOffset = allocation->submeshFirst * submeshStride;
        auto meshletOffset = allocation->meshletFirst * meshletStride;
        auto meshletVertexOffset = allocation->meshletVertexFirst * meshletVertexStride;
        auto meshletIndexOffset = ((size_t)allocation->meshletTriangleFirst * 3ull);
        auto positionsOffset = allocation->vertexFirst * positionsStride;
        auto attributesOffset = allocation->vertexFirst * attributesStride;
        auto indexOffset = allocation->indexFirst * indexStride;

        auto submeshesSize = allocation->submeshCount * submeshStride;
        auto meshletsSize = allocation->meshletCount * meshletStride;
        auto meshletVerticesSize = allocation->meshletVertexCount * meshletVertexStride;
        auto meshletIndicesSize = ((size_t)allocation->meshletTriangleCount * 3ull);
        auto positionsSize = allocation->vertexCount * positionsStride;
        auto attributesSize = allocation->vertexCount * attributesStride;
        auto indicesSize = allocation->indexCount * indexStride;

        m_submeshBuffer->SparseDeallocate({ submeshOffset, submeshesSize });
        m_meshletBuffer->SparseDeallocate({ meshletOffset, meshletsSize });
        m_meshletVertexBuffer->SparseDeallocate({ meshletVertexOffset, meshletVerticesSize });
        m_meshletIndexBuffer->SparseDeallocate({ meshletIndexOffset, meshletIndicesSize });
        m_vertexBuffers[0]->SparseDeallocate({ attributesOffset, attributesSize });
        m_vertexBuffers[1]->SparseDeallocate({ positionsOffset, positionsSize });
        m_indexBuffer->SparseDeallocate({ indexOffset, indicesSize });

        m_submeshCount -= allocation->submeshCount;
        m_meshletCount -= allocation->meshletCount;
        m_meshletVertexCount -= allocation->meshletVertexCount;
        m_meshletTriangleCount -= allocation->meshletTriangleCount;
        m_vertexCount -= allocation->vertexCount;
        m_indexCount -= allocation->indexCount;
        m_preferredIndex = m_allocations.GetIndex(allocation);

        for (auto i = 0u; i < allocation->submeshCount; ++i)
        {
            auto submeshIndex = allocation->submeshFirst + i;
            m_submeshes.Delete(submeshIndex);
        }

        m_allocations.Delete(allocation);
    }

    bool MeshStaticAllocator::GatherRayTracingGeometry(uint32_t globalSubmeshIndex, RayTracingGeometryInfo* outInfo) const
    {
        if (!HasPendingUpload())
        {
            const auto& sm = GetSubmesh(globalSubmeshIndex);
            outInfo->name = sm.name;
            outInfo->vertexBuffer = m_vertexBuffers[1].get();
            outInfo->indexBuffer = m_indexBuffer.get();
            outInfo->vertexOffset = 0u;
            outInfo->vertexStride = m_streamLayout.GetStride(1u);
            outInfo->vertexFirst = sm.vertexFirst;
            outInfo->vertexCount = sm.vertexCount;
            outInfo->indexStride = RHIEnumConvert::Size(m_indexType);
            outInfo->indexFirst = sm.indexFirst;
            outInfo->indexCount = sm.indexCount;
            outInfo->customIndex = 0u;
            return true;
        }

        return false;
    }


    MeshStatic::MeshStatic(MeshStaticAllocator* allocator, const char* filepath)
    {
        PKAssets::PKAsset asset;

        PK_FATAL_ASSERT(PKAssets::OpenAsset(filepath, &asset) == 0, "Failed to open asset at path: %s", filepath);
        PK_FATAL_ASSERT(asset.header->type == PKAssets::PKAssetType::Mesh, "Trying to read a mesh from a non mesh file!")

        auto mesh = PKAssets::ReadAsMesh(&asset);
        auto base = asset.rawData;

        PK_FATAL_ASSERT(mesh->vertexAttributeCount > 0, "Trying to read a mesh with 0 vertex attributes!");
        PK_FATAL_ASSERT(mesh->vertexCount > 0, "Trying to read a shader with 0 vertices!");
        PK_FATAL_ASSERT(mesh->indexCount > 0, "Trying to read a shader with 0 indices!");
        PK_FATAL_ASSERT(mesh->submeshCount > 0, "Trying to read a shader with 0 submeshes!");

        auto pAttributes = mesh->vertexAttributes.Get(base);
        auto pVertices = mesh->vertexBuffer.Get(base);
        auto pIndices = mesh->indexBuffer.Get(base);
        auto pSubmeshes = mesh->submeshes.Get(base);

        auto* submeshes = PK_STACK_ALLOC(SubMesh, mesh->submeshCount);

        for (auto i = 0u; i < mesh->submeshCount; ++i)
        {
            submeshes[i].name = 0u;
            submeshes[i].vertexFirst = 0u;
            submeshes[i].vertexCount = mesh->vertexCount;
            submeshes[i].indexFirst = pSubmeshes[i].firstIndex;
            submeshes[i].indexCount = pSubmeshes[i].indexCount;
            submeshes[i].meshletFirst = 0u;
            submeshes[i].meshletCount = 0u;
            submeshes[i].bounds = AABB<float3>(float3(pSubmeshes[i].bbmin), float3(pSubmeshes[i].bbmax));
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
            PK_FATAL_ASSERT(allocator, "Cannot create a virtual mesh without an allocator!");

            MeshStaticDescriptor desc{};
            desc.name = String::ToFilePathStem<64>(filepath).c_str();

            desc.regular.pVertices = pVertices;
            desc.regular.pIndices = pIndices;
            desc.regular.streamLayout = streamLayout;
            desc.regular.pSubmeshes = submeshes;
            desc.regular.indexType = mesh->indexType;
            desc.regular.vertexCount = mesh->vertexCount;
            desc.regular.indexCount = mesh->indexCount;
            desc.regular.submeshCount = mesh->submeshCount;

            auto meshletMesh = mesh->meshletMesh.Get(base);
            desc.meshlets.pSubmeshes = meshletMesh->submeshes.Get(base);
            desc.meshlets.submeshCount = meshletMesh->submeshCount;
            desc.meshlets.pMeshlets = meshletMesh->meshlets.Get(base);
            desc.meshlets.meshletCount = meshletMesh->meshletCount;
            desc.meshlets.pVertices = meshletMesh->vertices.Get(base);
            desc.meshlets.vertexCount = meshletMesh->vertexCount;
            desc.meshlets.pIndices = meshletMesh->indices.Get(base);
            desc.meshlets.triangleCount = meshletMesh->triangleCount;
            m_allocation = allocator->Allocate(desc);
        }

        PKAssets::CloseAsset(&asset);
    }

    MeshStatic::MeshStatic(MeshStatic&& other)
    {
        if (&other != this)
        {
            m_allocation = other.m_allocation;
            other.m_allocation = nullptr;
        }
    }

    MeshStatic::~MeshStatic()
    {
        if (m_allocation)
        {
            m_allocation->allocator->Deallocate(m_allocation);
            m_allocation = nullptr;
        }
    }

    bool MeshStatic::GatherRayTracingGeometry(uint32_t localIndex, RayTracingGeometryInfo* outInfo) const
    {
        return m_allocation->allocator->GatherRayTracingGeometry(GetGlobalSubmeshIndex(localIndex), outInfo);
    }


    Mesh::Mesh(const char* filepath)
    {
        PKAssets::PKAsset asset;

        PK_FATAL_ASSERT(PKAssets::OpenAsset(filepath, &asset) == 0, "Failed to open asset at path: %s", filepath);
        PK_FATAL_ASSERT(asset.header->type == PKAssets::PKAssetType::Mesh, "Trying to read a mesh from a non mesh file!")

        auto mesh = PKAssets::ReadAsMesh(&asset);
        auto base = asset.rawData;

        PK_FATAL_ASSERT(mesh->vertexAttributeCount > 0, "Trying to read a mesh with 0 vertex attributes!");
        PK_FATAL_ASSERT(mesh->vertexAttributeCount <= PK_RHI_MAX_VERTEX_ATTRIBUTES, "Trying to read a mesh with more than maximum allowed vertex attributes!");
        PK_FATAL_ASSERT(mesh->vertexCount > 0, "Trying to read a shader with 0 vertices!");
        PK_FATAL_ASSERT(mesh->indexCount > 0, "Trying to read a shader with 0 indices!");
        PK_FATAL_ASSERT(mesh->submeshCount > 0, "Trying to read a shader with 0 submeshes!");

        const auto pAttributes = mesh->vertexAttributes.Get(base);
        const auto pSubmeshes = mesh->submeshes.Get(base);
        const auto fileName = String::ToFilePathStem<64>(filepath);

        MeshDescriptor descriptor{};
        descriptor.pVertices = mesh->vertexBuffer.Get(base);
        descriptor.pIndices = mesh->indexBuffer.Get(base);
        descriptor.pSubmeshes = PK_STACK_ALLOC(SubMesh, mesh->submeshCount);
        descriptor.indexType = mesh->indexType;
        descriptor.vertexCount = mesh->vertexCount;
        descriptor.indexCount = mesh->indexCount;
        descriptor.submeshCount = mesh->submeshCount;

        for (auto i = 0u; i < mesh->submeshCount; ++i)
        {
            descriptor.pSubmeshes[i].name = FixedString128("%s.Submesh%u", fileName.c_str(), i).c_str();
            descriptor.pSubmeshes[i].vertexFirst = 0u;
            descriptor.pSubmeshes[i].vertexCount = mesh->vertexCount;
            descriptor.pSubmeshes[i].indexFirst = pSubmeshes[i].firstIndex;
            descriptor.pSubmeshes[i].indexCount = pSubmeshes[i].indexCount;
            descriptor.pSubmeshes[i].meshletFirst = 0u;
            descriptor.pSubmeshes[i].meshletCount = 0u;
            descriptor.pSubmeshes[i].bounds = AABB<float3>(float3(pSubmeshes[i].bbmin), float3(pSubmeshes[i].bbmax));
        }

        for (auto i = 0u; i < mesh->vertexAttributeCount; ++i)
        {
            auto attribute = descriptor.streamLayout.Add();
            attribute->stream = (uint8_t)pAttributes[i].stream;
            attribute->inputRate = InputRate::PerVertex;
            attribute->stride = 0u;
            attribute->offset = pAttributes[i].offset;
            attribute->size = pAttributes[i].size;
            attribute->name = pAttributes[i].name;
        }

        descriptor.streamLayout.CalculateOffsetsAndStride();

        SetResources(descriptor, fileName);
        PKAssets::CloseAsset(&asset);
    }

    Mesh::Mesh(const MeshDescriptor& descriptor, const char* name)
    {
        SetResources(descriptor, name);
    }

    Mesh::Mesh(const RHIBufferRef& indexBuffer,
        ElementType indexType,
        RHIBufferRef* vertexBuffers,
        uint32_t vertexBufferCount,
        const VertexStreamLayout& streamLayout,
        SubMesh* submeshes,
        uint32_t submeshCount)
    {
        SetResources(indexBuffer, indexType, vertexBuffers, vertexBufferCount, streamLayout, submeshes, submeshCount);
    }

    void Mesh::SetResources(const MeshDescriptor& desc, const char* name)
    {
        RHIBufferRef vertexBuffers[PK_RHI_MAX_VERTEX_ATTRIBUTES];
        FixedString128 bufferNames[PK_RHI_MAX_VERTEX_ATTRIBUTES]{};
        FixedString128 vertexBufferName({ name, ".VertexBuffer" });
        FixedString128 indexBufferName({ name, ".IndexBuffer" });

        auto commandBuffer = CommandBufferExt(RHI::GetCommandBuffer(QueueType::Transfer));
        
        auto pVertices = (char*)desc.pVertices;
        auto bufferCount = 0u;

        for (auto& attribute : desc.streamLayout)
        {
            bufferNames[attribute.stream].Append('.');
            bufferNames[attribute.stream].Append(attribute.name);
        }

        for (; bufferCount < PK_RHI_MAX_VERTEX_ATTRIBUTES && desc.streamLayout.GetStride(bufferCount) != 0u; ++bufferCount)
        {
            auto size = desc.streamLayout.GetStride(bufferCount) * desc.vertexCount;
            vertexBuffers[bufferCount] = RHI::CreateBuffer(size, BufferUsage::DefaultVertex, FixedString128({ vertexBufferName.c_str(), bufferNames[bufferCount].c_str() }));
            commandBuffer.UploadBufferData(vertexBuffers[bufferCount].get(), pVertices);
            pVertices += size;
        }

        auto indicesSize = PKAssets::PKElementTypeToSize(desc.indexType) * desc.indexCount;
        auto indexBuffer = RHI::CreateBuffer(indicesSize, BufferUsage::DefaultIndex, indexBufferName.c_str());
        commandBuffer.UploadBufferData(indexBuffer.get(), desc.pIndices);

        SetResources(indexBuffer, desc.indexType, vertexBuffers, bufferCount, desc.streamLayout, desc.pSubmeshes, desc.submeshCount);

        m_uploadFence = commandBuffer->GetFenceRef();
    }

    void Mesh::SetResources(const RHIBufferRef& indexBuffer,
        ElementType indexType,
        RHIBufferRef* vertexBuffers,
        uint32_t vertexBufferCount,
        const VertexStreamLayout& streamLayout,
        SubMesh* submeshes,
        uint32_t submeshCount)
    {
        m_indexBuffer = indexBuffer;
        m_indexType = indexType;
        m_streamLayout = streamLayout;
        m_vertexBuffers.Clear();

        for (auto i = 0u; i < vertexBufferCount; ++i)
        {
            m_vertexBuffers.Add(vertexBuffers[i]);
        }

        auto vertexPositionName = NameID(PK_RHI_VS_POSITION);

        for (auto i = 0u; i < m_streamLayout.GetCount(); ++i)
        {
            if (m_streamLayout[i].name == vertexPositionName && m_streamLayout[i].size == (uint16_t)sizeof(float3))
            {
                m_positionAttributeIndex = i;
            }
        }

        m_fullrange = SubMesh();
        m_submeshes.Resize(submeshCount);

        for (auto i = 0u; i < submeshCount; ++i)
        {
            m_submeshes[i] = submeshes[i];
            m_fullrange.bounds |= submeshes[i].bounds;
            m_fullrange.vertexCount = math::max(m_fullrange.vertexCount, submeshes[i].vertexFirst + submeshes[i].vertexCount);
            m_fullrange.indexCount = math::max(m_fullrange.indexCount, submeshes[i].indexFirst + submeshes[i].indexCount);
        }
    }

    const SubMesh& Mesh::GetSubmesh(int32_t submesh) const
    {
        if (submesh >= 0 && m_submeshes.GetCount())
        {
            return m_submeshes[math::min(submesh, (int)m_submeshes.GetCount())];
        }

        return m_fullrange;
    }

    bool Mesh::GatherRayTracingGeometry(uint32_t submesh, RayTracingGeometryInfo* outInfo) const
    {
        if (!HasPendingUpload() && m_positionAttributeIndex != ~0u)
        {
            auto& sm = GetSubmesh(submesh);
            auto positionStream = &m_streamLayout[m_positionAttributeIndex];
            outInfo->name = sm.name;
            outInfo->vertexBuffer = m_vertexBuffers[positionStream->stream].get();
            outInfo->indexBuffer = m_indexBuffer.get();
            outInfo->vertexOffset = positionStream->offset;
            outInfo->vertexStride = positionStream->stride;
            outInfo->vertexFirst = sm.vertexFirst;
            outInfo->vertexCount = sm.vertexCount;
            outInfo->indexStride = RHIEnumConvert::Size(m_indexType);
            outInfo->indexFirst = sm.indexFirst;
            outInfo->indexCount = sm.indexCount;
            outInfo->customIndex = 0u;
            return true;
        }

        return false;
    }
}

template<>
const char* PK::Asset::GetExtension<PK::Mesh>() { return "*.pkmesh"; }

template<>
const char* PK::Asset::GetExtension<PK::MeshStatic>() { return "*.pkmesh"; }

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
        m_streamLayout = VertexStreamLayout(
        {
            { ElementType::Half4, PK_RHI_VS_NORMAL, 0 },
            { ElementType::Half4, PK_RHI_VS_TANGENT, 0 },
            { ElementType::Half2, PK_RHI_VS_TEXCOORD0, 0 },
            { ElementType::Float3, PK_RHI_VS_POSITION, 1 },
        });

        m_vertexBuffers.ClearFast();
        m_vertexBuffers.Add(RHI::CreateBuffer(m_streamLayout.GetStride(0u) * 2000000u, BufferUsage::DefaultVertex, "MeshStaticCollection.VertexAttributes"));
        m_vertexBuffers.Add(RHI::CreateBuffer(m_streamLayout.GetStride(1u) * 2000000u, BufferUsage::DefaultVertex | BufferUsage::Storage, "MeshStaticCollection.VertexPositions"));
        m_indexBuffer = RHI::CreateBuffer(m_indexSize * 2000000u, BufferUsage::DefaultIndex | BufferUsage::Storage, "MeshStaticCollection.IndexBuffer");
        m_allocatorVertices.ClearAndReserve(m_vertexBuffers[0]->GetSize(), 128u);
        m_allocatorIndices.ClearAndReserve(m_indexBuffer->GetSize(), 128u);

        const auto meshletBufferSize = 67108864ull; // 64MB
        m_meshletBuffer = RHI::CreateBuffer(meshletBufferSize, BufferUsage::DefaultStorage, "Meshlet.DataBuffer");
        m_allocatorMeshlets.ClearAndReserve(m_meshletBuffer->GetSize(), 256u);
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

        const auto stridePositions = m_streamLayout.GetStride(1u);
        const auto strideAttributes = m_streamLayout.GetStride(0u);
        const auto strideIndex = m_indexSize;

        const auto sizePositions = desc.regular.vertexCount * stridePositions;
        const auto sizeAttributes = desc.regular.vertexCount * strideAttributes;
        const auto sizeIndices = desc.regular.indexCount * strideIndex;

        PK_FATAL_ASSERT(m_allocatorVertices.Allocate(desc.regular.vertexCount, 1ull, allocation->allocVertices), "Failed to allocate mesh vertices!");
        const auto vertexFirst = allocation->allocVertices.offset;
        const auto offsetPositions = vertexFirst * stridePositions;
        const auto offsetAttributes = vertexFirst * strideAttributes;

        PK_FATAL_ASSERT(m_allocatorIndices.Allocate(desc.regular.indexCount, 1ull, allocation->allocIndices), "Failed to allocate mesh indices!");
        const auto indexFirst = allocation->allocIndices.offset;
        const auto offsetIndices = indexFirst * strideIndex;

        const auto strideMeshletSubmesh = sizeof(PKAssets::PKMeshletSubmesh);
        const auto strideMeshlet = sizeof(PKAssets::PKMeshlet);
        const auto strideMeshletVertex = sizeof(PKAssets::PKMeshletVertex);

        const auto sizeMeshletSubmeshes = desc.meshlets.submeshCount * strideMeshletSubmesh;
        const auto sizeMeshlets = desc.meshlets.meshletCount * strideMeshlet;
        const auto sizeMeshletVertices = desc.meshlets.vertexCount * strideMeshletVertex;
        const auto sizeMeshletIndices = math::align((size_t)desc.meshlets.triangleCount * 3ull, 4ull);
        
        auto allocationSizeMeshlets = 0u;
        allocationSizeMeshlets += sizeMeshletSubmeshes + strideMeshletSubmesh - 1ull;
        allocationSizeMeshlets += sizeMeshlets         + strideMeshlet - 1ull;
        allocationSizeMeshlets += sizeMeshletVertices  + strideMeshletVertex - 1ull;
        allocationSizeMeshlets += sizeMeshletIndices   + 12ull - 1ull;
        PK_FATAL_ASSERT(m_allocatorMeshlets.Allocate(allocationSizeMeshlets, 16ull, allocation->allocMeshlets), "Failed to allocate mesh meshlets!");

        const auto offsetMeshletBase    = allocation->allocMeshlets.offset;
        const auto offsetMeshletSubmesh = math::align(offsetMeshletBase    + 0ull,                 strideMeshletSubmesh);
        const auto offsetMeshlet        = math::align(offsetMeshletSubmesh + sizeMeshletSubmeshes, strideMeshlet);
        const auto offsetMeshletVertex  = math::align(offsetMeshlet        + sizeMeshlets,         strideMeshletVertex);
        const auto offsetMeshletIndex   = math::align(offsetMeshletVertex  + sizeMeshletVertices,  12ull);

        const auto meshletSubmeshFirst  = (uint32_t)(offsetMeshletSubmesh / strideMeshletSubmesh);
        const auto meshletFirst         = (uint32_t)(offsetMeshlet / strideMeshlet);
        const auto meshletVertexFirst   = (uint32_t)(offsetMeshletVertex / strideMeshletVertex);
        const auto meshletTriangleFirst = (uint32_t)(offsetMeshletIndex / 3ull);

        auto submeshes = m_submeshes.NewArray(desc.meshlets.submeshCount);
        allocation->submeshFirst = m_submeshes.GetIndex(submeshes);
        allocation->submeshCount = desc.meshlets.submeshCount;

        allocation->name = desc.name;

        for (auto i = 0u; i < allocation->submeshCount; ++i)
        {
            desc.meshlets.pSubmeshes[i].firstMeshlet += meshletFirst;

            // GPU Meshlet submeshes are not in a contiguous block. 
            // This index is needed to address them correctly.
            submeshes[i].meshletSubmesh = meshletSubmeshFirst + i;
            submeshes[i].meshletFirst = desc.meshlets.pSubmeshes[i].firstMeshlet;
            submeshes[i].meshletCount = desc.meshlets.pSubmeshes[i].meshletCount;
            submeshes[i].vertexFirst = desc.regular.pSubmeshes[i].vertexFirst + vertexFirst;
            submeshes[i].vertexCount = desc.regular.pSubmeshes[i].vertexCount;
            submeshes[i].indexFirst = desc.regular.pSubmeshes[i].indexFirst + indexFirst;
            submeshes[i].indexCount = desc.regular.pSubmeshes[i].indexCount;
            submeshes[i].bounds = desc.regular.pSubmeshes[i].bounds;
            submeshes[i].name = FixedString128("%s.Submesh%u", desc.name.c_str(), i).c_str();
        }

        for (auto i = 0u; i < desc.meshlets.meshletCount; ++i)
        {
            desc.meshlets.pMeshlets[i].vertexFirst += meshletVertexFirst;
            desc.meshlets.pMeshlets[i].triangleFirst += meshletTriangleFirst;
        }

        auto commandBuffer = CommandBufferExt(RHI::GetCommandBuffer(QueueType::Transfer));
        commandBuffer.UploadBufferData(m_meshletBuffer.get(), desc.meshlets.pSubmeshes, offsetMeshletSubmesh,   sizeMeshletSubmeshes);
        commandBuffer.UploadBufferData(m_meshletBuffer.get(), desc.meshlets.pMeshlets,  offsetMeshlet,          sizeMeshlets);
        commandBuffer.UploadBufferData(m_meshletBuffer.get(), desc.meshlets.pVertices,  offsetMeshletVertex,    sizeMeshletVertices);
        commandBuffer.UploadBufferData(m_meshletBuffer.get(), desc.meshlets.pIndices,   offsetMeshletIndex,     sizeMeshletIndices);

        auto indexView = commandBuffer.BeginBufferWrite<uint8_t>(m_indexBuffer.get(), offsetIndices, sizeIndices);
        MeshUtilities::CopyIndexBuffer(indexView.data, desc.regular.pIndices, desc.regular.indexCount, desc.regular.indexSize, strideIndex);
        commandBuffer.EndBufferWrite(m_indexBuffer.get(), indexView);

        // Align vertices into split layout if necessary
        MeshUtilities::AlignVertexStreams(desc.regular.pVertices, desc.regular.vertexCount, desc.regular.streamLayout, m_streamLayout);
        commandBuffer.UploadBufferData(m_vertexBuffers[0].get(), (char*)desc.regular.pVertices + 0ull,           offsetAttributes, sizeAttributes);
        commandBuffer.UploadBufferData(m_vertexBuffers[1].get(), (char*)desc.regular.pVertices + sizeAttributes, offsetPositions,  sizePositions);

        m_uploadFence = commandBuffer->GetFenceRef();

        return allocation;
    }

    void MeshStaticAllocator::Deallocate(Allocation* allocation)
    {
        m_allocatorVertices.Free(allocation->allocVertices);
        m_allocatorIndices.Free(allocation->allocIndices);
        m_allocatorMeshlets.Free(allocation->allocMeshlets);
        m_preferredIndex = m_allocations.GetIndex(allocation);
        m_submeshes.Delete(m_submeshes.GetData() + allocation->submeshFirst, allocation->submeshCount);
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
            outInfo->indexStride = m_indexSize;
            outInfo->indexFirst = sm.indexFirst;
            outInfo->indexCount = sm.indexCount;
            outInfo->customIndex = 0u;
            return true;
        }

        return false;
    }


    MeshStatic::MeshStatic(MeshStaticAllocator* allocator, const char* filepath)
    {
        PKAssets::PKAsset asset{};
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
            submeshes[i].meshletSubmesh = 0u;
            submeshes[i].meshletFirst = 0u;
            submeshes[i].meshletCount = 0u;
            submeshes[i].bounds = AABB<float3>(float3(pSubmeshes[i].bbmin), float3(pSubmeshes[i].bbmax));
        }

        VertexStreamLayout streamLayout;
        for (auto i = 0u; i < mesh->vertexAttributeCount; ++i)
        {
            auto stream = streamLayout.Add();
            stream->name = pAttributes[i].name;
            stream->stream = pAttributes[i].stream;
            stream->inputRate = InputRate::PerVertex;
            stream->stride = 0u;
            stream->offset = pAttributes[i].offset;
            stream->format = (ElementType)pAttributes[i].format;
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
            desc.regular.indexSize = mesh->indexSize;
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
        PKAssets::PKAsset asset{};
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
        descriptor.indexSize = mesh->indexSize;
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
            descriptor.pSubmeshes[i].meshletSubmesh = 0u;
            descriptor.pSubmeshes[i].meshletFirst = 0u;
            descriptor.pSubmeshes[i].meshletCount = 0u;
            descriptor.pSubmeshes[i].bounds = AABB<float3>(float3(pSubmeshes[i].bbmin), float3(pSubmeshes[i].bbmax));
        }

        for (auto i = 0u; i < mesh->vertexAttributeCount; ++i)
        {
            auto attribute = descriptor.streamLayout.Add();
            attribute->stream = pAttributes[i].stream;
            attribute->inputRate = InputRate::PerVertex;
            attribute->stride = 0u;
            attribute->offset = pAttributes[i].offset;
            attribute->format = (ElementType)pAttributes[i].format;
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
        uint32_t indexSize,
        const VertexStreamLayout& streamLayout,
        RHIBufferRef* vertexBuffers,
        uint32_t vertexBufferCount,
        SubMesh* submeshes,
        uint32_t submeshCount)
    {
        SetResources(indexBuffer, indexSize, streamLayout, vertexBuffers, vertexBufferCount, submeshes, submeshCount);
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
            bufferNames[attribute.stream].Append(attribute.name.c_str());
        }

        for (; bufferCount < PK_RHI_MAX_VERTEX_ATTRIBUTES && desc.streamLayout.GetStride(bufferCount) != 0u; ++bufferCount)
        {
            auto size = desc.streamLayout.GetStride(bufferCount) * desc.vertexCount;
            vertexBuffers[bufferCount] = RHI::CreateBuffer(size, BufferUsage::DefaultVertex, FixedString128({ vertexBufferName.c_str(), bufferNames[bufferCount].c_str() }));
            commandBuffer.UploadBufferData(vertexBuffers[bufferCount].get(), pVertices);
            pVertices += size;
        }

        auto indexBuffer = RHI::CreateBuffer(desc.indexSize * desc.indexCount, BufferUsage::DefaultIndex, indexBufferName.c_str());
        commandBuffer.UploadBufferData(indexBuffer.get(), desc.pIndices);

        SetResources(indexBuffer, desc.indexSize, desc.streamLayout, vertexBuffers, bufferCount, desc.pSubmeshes, desc.submeshCount);

        m_uploadFence = commandBuffer->GetFenceRef();
    }

    void Mesh::SetResources(const RHIBufferRef& indexBuffer,
        uint32_t indexSize,
        const VertexStreamLayout& streamLayout,
        RHIBufferRef* vertexBuffers,
        uint32_t vertexBufferCount,
        SubMesh* submeshes,
        uint32_t submeshCount)
    {
        m_indexBuffer = indexBuffer;
        m_indexSize = indexSize;
        m_streamLayout = streamLayout;
        m_vertexBuffers.Clear();

        for (auto i = 0u; i < vertexBufferCount; ++i)
        {
            m_vertexBuffers.Add(vertexBuffers[i]);
        }

        auto vertexPositionName = NameID(PK_RHI_VS_POSITION);

        for (auto i = 0u; i < m_streamLayout.GetCount(); ++i)
        {
            if (m_streamLayout[i].name == vertexPositionName && m_streamLayout[i].format == ElementType::Float3)
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
            return m_submeshes[math::min(submesh, (int32_t)m_submeshes.GetCount())];
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
            outInfo->indexStride = m_indexSize;
            outInfo->indexFirst = sm.indexFirst;
            outInfo->indexCount = sm.indexCount;
            outInfo->customIndex = 0u;
            return true;
        }

        return false;
    }
}

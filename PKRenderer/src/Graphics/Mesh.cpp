#include "PrecompiledHeader.h"
#include <PKAssets/PKAssetLoader.h>
#include "Math/FunctionsIntersect.h"
#include "Math/FunctionsMisc.h"
#include "Core/CLI/Log.h"
#include "Graphics/RHI/RHICommandBuffer.h"
#include "Graphics/CommandBufferExt.h"
#include "Mesh.h"

using namespace PK::Math;
using namespace PK::Utilities;
using namespace PK::Core;
using namespace PK::Core::Assets;
using namespace PK::Graphics;
using namespace PK::Graphics::RHI;

namespace PK::Graphics
{
    Mesh::Mesh() {}

    Mesh::Mesh(const BufferRef& indexBuffer,
        ElementType indexType,
        BufferRef* vertexBuffers,
        uint32_t vertexBufferCount,
        const VertexStreamLayout& streamLayout,
        SubMesh* submeshes,
        uint32_t submeshCount)
    {
        SetResources(indexBuffer, indexType, vertexBuffers, vertexBufferCount, streamLayout, submeshes, submeshCount);
    }

    void Mesh::SetResources(const BufferRef& indexBuffer,
        ElementType indexType,
        BufferRef* vertexBuffers,
        uint32_t vertexBufferCount,
        const VertexStreamLayout& streamLayout,
        SubMesh* submeshes,
        uint32_t submeshCount)
    {
        m_indexBuffer = indexBuffer;
        m_indexType = indexType;
        m_vertexBuffers.ClearFull();
        m_streamLayout = streamLayout;

        for (auto i = 0u; i < vertexBufferCount; ++i)
        {
            m_vertexBuffers.Add(vertexBuffers[i]);
        }

        auto vertexPositionName = NameID(PK_VS_POSITION);

        for (auto i = 0u; i < m_streamLayout.GetCount(); ++i)
        {
            const auto& attribute = m_streamLayout[i];

            if (attribute.name == vertexPositionName &&
                attribute.stream < vertexBufferCount &&
                attribute.size == (uint16_t)sizeof(float3))
            {
                m_positionAttributeIndex = i;
            }
        }

        m_fullRange = SubMesh();
        m_submeshes.resize(submeshCount);

        for (auto i = 0u; i < submeshCount; ++i)
        {
            m_submeshes[i] = submeshes[i];
            Functions::BoundsEncapsulate(&m_fullRange.bounds, submeshes[i].bounds);
            m_fullRange.vertexCount = glm::max(m_fullRange.vertexCount, submeshes[i].vertexFirst + submeshes[i].vertexCount);
            m_fullRange.indexCount = glm::max(m_fullRange.indexCount, submeshes[i].indexFirst + submeshes[i].indexCount);
        }
    }

    void Mesh::AssetImport(const char* filepath)
    {
        PK::Assets::PKAsset asset;

        PK_THROW_ASSERT(PK::Assets::OpenAsset(filepath, &asset) == 0, "Failed to open asset at path: %s", filepath);
        PK_THROW_ASSERT(asset.header->type == PK::Assets::PKAssetType::Mesh, "Trying to read a mesh from a non mesh file!")

        auto mesh = PK::Assets::ReadAsMesh(&asset);
        auto base = asset.rawData;

        PK_THROW_ASSERT(mesh->vertexAttributeCount > 0, "Trying to read a mesh with 0 vertex attributes!");
        PK_THROW_ASSERT(mesh->vertexAttributeCount <= PK_MAX_VERTEX_ATTRIBUTES, "Trying to read a mesh with more than maximum allowed vertex attributes!");
        PK_THROW_ASSERT(mesh->vertexCount > 0, "Trying to read a shader with 0 vertices!");
        PK_THROW_ASSERT(mesh->indexCount > 0, "Trying to read a shader with 0 indices!");
        PK_THROW_ASSERT(mesh->submeshCount > 0, "Trying to read a shader with 0 submeshes!");

        auto pAttributes = mesh->vertexAttributes.Get(base);
        auto pVertices = mesh->vertexBuffer.Get(base);
        auto pIndexBuffer = mesh->indexBuffer.Get(base);
        auto pSubmeshes = mesh->submeshes.Get(base);

        std::vector<SubMesh> submeshes;

        for (auto i = 0u; i < mesh->submeshCount; ++i)
        {
            auto bounds = BoundingBox::MinMax(Functions::ToFloat3(pSubmeshes[i].bbmin), Functions::ToFloat3(pSubmeshes[i].bbmax));
            submeshes.push_back({ 0u, mesh->vertexCount, pSubmeshes[i].firstIndex, pSubmeshes[i].indexCount, bounds });
            Functions::BoundsEncapsulate(&m_fullRange.bounds, bounds);
        }

        VertexStreamLayout streamLayout;
        std::string bufferNames[PK_MAX_VERTEX_ATTRIBUTES]{};
        BufferRef vertexBuffers[PK_MAX_VERTEX_ATTRIBUTES];

        for (auto i = 0u; i < mesh->vertexAttributeCount; ++i)
        {
            auto attribute = streamLayout.Add();
            attribute->stream = (uint8_t)pAttributes[i].stream;
            attribute->inputRate = InputRate::PerVertex;
            attribute->stride = 0u;
            attribute->offset = pAttributes[i].offset;
            attribute->size = pAttributes[i].size;
            attribute->name = pAttributes[i].name;
            bufferNames[attribute->stream] += std::string(".") + std::string(pAttributes[i].name);
        }

        streamLayout.CalculateOffsetsAndStride();

        auto vertexBufferName = GetFileName() + std::string(".VertexBuffer");
        auto indexBufferName = GetFileName() + std::string(".IndexBuffer");
        auto commandBuffer = CommandBufferExt(RHIGetCommandBuffer(QueueType::Transfer));

        auto pVerticesOffset = (char*)pVertices;
        auto bufferCount = 0u;

        for (auto i = 0u; i < PK_MAX_VERTEX_ATTRIBUTES && streamLayout.GetStride(i) != 0u; ++i)
        {
            auto stride = streamLayout.GetStride(i);
            auto bufferName = vertexBufferName + bufferNames[i];
            vertexBuffers[bufferCount] = RHICreateBuffer(stride * mesh->vertexCount, BufferUsage::DefaultVertex, bufferName.c_str());
            commandBuffer.UploadBufferData(vertexBuffers[bufferCount].get(), pVerticesOffset);
            pVerticesOffset += stride * mesh->vertexCount;
            bufferCount++;
        }

        auto indexStride = GetElementSize(mesh->indexType);
        auto indexBuffer = RHICreateBuffer(indexStride * mesh->indexCount, BufferUsage::DefaultIndex, indexBufferName.c_str());
        commandBuffer.UploadBufferData(m_indexBuffer.get(), pIndexBuffer);

        SetResources(indexBuffer,
            mesh->indexType,
            vertexBuffers,
            bufferCount,
            streamLayout,
            submeshes.data(),
            (uint32_t)submeshes.size());

        m_uploadFence = commandBuffer->GetFenceRef();

        PK::Assets::CloseAsset(&asset);
    }

    bool Mesh::TryGetAccelerationStructureGeometryInfo(uint32_t submesh, RHI::AccelerationStructureGeometryInfo* outInfo)
    {
        if (HasPendingUpload() || m_positionAttributeIndex == ~0u)
        {
            return false;
        }

        auto& sm = GetSubmesh(submesh);
        auto positionStream = &m_streamLayout[m_positionAttributeIndex];
        outInfo->name = GetAssetID();
        outInfo->vertexBuffer = m_vertexBuffers[positionStream->stream].get();
        outInfo->indexBuffer = m_indexBuffer.get();
        outInfo->vertexOffset = positionStream->offset;
        outInfo->vertexStride = positionStream->stride;
        outInfo->vertexFirst = sm.vertexFirst;
        outInfo->vertexCount = sm.vertexCount;
        outInfo->indexStride = GetElementSize(m_indexType);
        outInfo->indexFirst = sm.indexFirst;
        outInfo->indexCount = sm.indexCount;
        outInfo->customIndex = 0u;
        return true;
    }

    const Mesh::SubMesh& Mesh::GetSubmesh(int32_t submesh) const
    {
        if (submesh < 0 || m_submeshes.empty())
        {
            return m_fullRange;
        }

        auto idx = glm::min((uint)submesh, (uint)m_submeshes.size());
        return m_submeshes.at(idx);
    }

    const Mesh::VertexBuffers& Mesh::GetVertexBuffers() const { return m_vertexBuffers; }
    const VertexStreamLayout& Mesh::GetVertexStreamLayout() const { return m_streamLayout; }
    const ElementType Mesh::GetIndexType() const { return m_indexType; }
    const Buffer* Mesh::GetIndexBuffer() const { return m_indexBuffer.get(); }
    const uint32_t Mesh::GetSubmeshCount() const { return glm::max(1u, (uint32_t)m_submeshes.size()); }
    const Mesh::SubMesh& Mesh::GetFullRange() const { return m_fullRange; }
    const bool Mesh::HasPendingUpload() const { return !m_uploadFence.WaitInvalidate(0ull); }
}

template<>
bool Asset::IsValidExtension<Mesh>(const std::string& extension) { return extension.compare(".pkmesh") == 0; }

template<>
Ref<Mesh> Asset::Create() { return CreateRef<Mesh>(); }
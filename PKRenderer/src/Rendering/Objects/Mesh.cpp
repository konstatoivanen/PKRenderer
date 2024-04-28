#include "PrecompiledHeader.h"
#include <PKAssets/PKAssetLoader.h>
#include "Math/FunctionsIntersect.h"
#include "Math/FunctionsMisc.h"
#include "Core/Services/StringHashID.h"
#include "Rendering/RHI/Objects/CommandBuffer.h"
#include "Rendering/RHI/GraphicsAPI.h"
#include "Mesh.h"

using namespace PK::Math;
using namespace PK::Utilities;
using namespace PK::Core;
using namespace PK::Core::Services;
using namespace PK::Core::Assets;
using namespace PK::Rendering;
using namespace PK::Rendering::Objects;
using namespace PK::Rendering::RHI;
using namespace PK::Rendering::RHI::Objects;

namespace PK::Rendering::Objects
{
    Mesh::Mesh() {}

    Mesh::Mesh(const BufferRef& indexBuffer, BufferRef* vertexBuffers, uint32_t vertexBufferCount, SubMesh* submeshes, uint32_t submeshCount)
    {
        SetResources(indexBuffer, vertexBuffers, vertexBufferCount, submeshes, submeshCount);
    }

    void Mesh::SetResources(const BufferRef& indexBuffer, BufferRef* vertexBuffers, uint32_t vertexBufferCount, SubMesh* submeshes, uint32_t submeshCount)
    {
        m_indexBuffer = indexBuffer;
        m_vertexBuffers.ClearFull();

        for (auto i = 0u; i < vertexBufferCount; ++i)
        {
            m_vertexBuffers.Add(vertexBuffers[i]);
        }

        auto vertexPositionHash = StringHashID::StringToID(PK_VS_POSITION);

        for (auto i = 0u; i < vertexBufferCount; ++i)
        {
            uint32_t positionIndex = 0u;
            auto vertexBuffer = m_vertexBuffers[i]->get();
            auto vertexElement = vertexBuffer->GetLayout().TryGetElement(vertexPositionHash, &positionIndex);

            if (vertexElement != nullptr && vertexElement->Type == ElementType::Float3)
            {
                m_vertexPositionBufferIndex = i;
                m_vertexPositionOffset = vertexElement->Offset;
                break;
            }
        }

        m_fullRange = SubMesh();
        m_submeshes.resize(submeshCount);

        for (auto i = 0u; i < submeshCount; ++i)
        {
            m_submeshes[i] = submeshes[i];
            Functions::BoundsEncapsulate(&m_fullRange.bounds, submeshes[i].bounds);
            m_fullRange.vertexCount = glm::max(m_fullRange.vertexCount, submeshes[i].firstVertex + submeshes[i].vertexCount);
            m_fullRange.indexCount = glm::max(m_fullRange.indexCount, submeshes[i].firstIndex + submeshes[i].indexCount);
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
        PK_THROW_ASSERT(mesh->vertexCount > 0, "Trying to read a shader with 0 vertices!");
        PK_THROW_ASSERT(mesh->indexCount > 0, "Trying to read a shader with 0 indices!");
        PK_THROW_ASSERT(mesh->submeshCount > 0, "Trying to read a shader with 0 submeshes!");

        auto pAttributes = mesh->vertexAttributes.Get(base);
        auto pVertices = mesh->vertexBuffer.Get(base);
        auto pIndexBuffer = mesh->indexBuffer.Get(base);
        auto pSubmeshes = mesh->submeshes.Get(base);

        std::map<uint32_t, std::vector<BufferElement>> layoutMap;
        std::vector<SubMesh> submeshes;

        for (auto i = 0u; i < mesh->submeshCount; ++i)
        {
            auto bounds = BoundingBox::MinMax(Functions::ToFloat3(pSubmeshes[i].bbmin), Functions::ToFloat3(pSubmeshes[i].bbmax));
            submeshes.push_back({ 0u, mesh->vertexCount, pSubmeshes[i].firstIndex, pSubmeshes[i].indexCount, bounds });
            Functions::BoundsEncapsulate(&m_fullRange.bounds, bounds);
        }

        for (auto i = 0u; i < mesh->vertexAttributeCount; ++i)
        {
            layoutMap[pAttributes[i].stream].emplace_back(pAttributes[i].type, std::string(pAttributes[i].name));
        }

        auto pBufferOffset = 0ull;
        auto vertexBufferName = GetFileName() + std::string(".VertexBuffer");
        auto indexBufferName = GetFileName() + std::string(".IndexBuffer");
        auto cmd = GraphicsAPI::GetCommandBuffer(QueueType::Transfer);

        BufferRef vertexBuffers[PK_MAX_VERTEX_ATTRIBUTES];
        auto bufferCount = 0u;

        for (auto& kv : layoutMap)
        {
            vertexBuffers[bufferCount] = Buffer::Create(BufferLayout(kv.second), mesh->vertexCount, BufferUsage::DefaultVertex, vertexBufferName.c_str());
            cmd->UploadBufferData(vertexBuffers[bufferCount].get(), (char*)pVertices + pBufferOffset);
            pBufferOffset += vertexBuffers[bufferCount]->GetLayout().GetStride() * mesh->vertexCount;
            bufferCount++;
        }

        auto indexBuffer = Buffer::Create(mesh->indexType, mesh->indexCount, BufferUsage::DefaultIndex, indexBufferName.c_str());
        cmd->UploadBufferData(m_indexBuffer.get(), (char*)pVertices + pBufferOffset);

        SetResources(indexBuffer, vertexBuffers, bufferCount, submeshes.data(), (uint32_t)submeshes.size());

        m_uploadFence = cmd->GetFenceRef();

        PK::Assets::CloseAsset(&asset);
    }

    bool Mesh::TryGetAccelerationStructureGeometryInfo(uint32_t submesh, RHI::Objects::AccelerationStructureGeometryInfo* outInfo)
    {
        if (HasPendingUpload() || m_vertexPositionBufferIndex == ~0u)
        {
            return false;
        }

        auto& sm = GetSubmesh(submesh);
        outInfo->vertexBuffer = m_vertexBuffers[m_vertexPositionBufferIndex]->get();
        outInfo->indexBuffer = m_indexBuffer.get();
        outInfo->vertexOffset = m_vertexPositionOffset;
        outInfo->firstVertex = sm.firstVertex;
        outInfo->vertexCount = sm.vertexCount;
        outInfo->firstIndex = sm.firstIndex;
        outInfo->indexCount = sm.indexCount;
        outInfo->customIndex = 0u;
        outInfo->nameHashId = 0u; //@TODO fill this with something.
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
}

template<>
bool Asset::IsValidExtension<Mesh>(const std::string& extension) { return extension.compare(".pkmesh") == 0; }

template<>
Ref<Mesh> Asset::Create() { return CreateRef<Mesh>(); }
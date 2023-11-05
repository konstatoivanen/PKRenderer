#include "PrecompiledHeader.h"
#include <PKAssets/PKAssetLoader.h>
#include "Math/FunctionsIntersect.h"
#include "Math/FunctionsMisc.h"
#include "Core/Services/StringHashID.h"
#include "Rendering/RHI/GraphicsAPI.h"
#include "Mesh.h"

using namespace PK::Math;
using namespace PK::Core;
using namespace PK::Core::Services;
using namespace PK::Utilities;
using namespace PK::Rendering;
using namespace PK::Rendering::Objects;
using namespace PK::Rendering::RHI;
using namespace PK::Rendering::RHI::Objects;

namespace PK::Rendering::Objects
{
    static void AlignVertices(char* vertices, size_t vcount, const BufferLayout& layout, const std::vector<const BufferLayout*> targetLayouts)
    {
        auto stride = layout.GetStride();
        auto fullStride = 0ull;
        auto needsAlignment = false;
        auto streamIndex = 0u;

        for (auto& targetLayout : targetLayouts)
        {
            for (auto& element : *targetLayout)
            {
                uint32_t elementIndex = 0u;
                auto other = layout.TryGetElement(element.NameHashId, &elementIndex);
                PK_THROW_ASSERT(other, "Required element not present in source layout!");
                PK_THROW_ASSERT(other->Type == element.Type, "Element type missmatch!");
                needsAlignment |= other->Location != streamIndex || other->Offset != element.Offset;
            }

            fullStride += targetLayout->GetStride();
            ++streamIndex;
        }

        PK_THROW_ASSERT(fullStride == stride, "Layout stride missmatch!");

        if (!needsAlignment)
        {
            return;
        }

        auto buffer = (char*)calloc(vcount, stride);
        auto subBuffer = buffer;

        for (auto& targetLayout : targetLayouts)
        {
            auto targetStride = targetLayout->GetStride();

            for (auto& targetElement : *targetLayout)
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

    static void FindAllocationRange(const std::vector<SubMesh>& submeshes, SubMesh* range)
    {
        // @TODO refactor this to be more memory efficient
        auto sortedSubmeshes = std::vector<SubMesh>(submeshes);
        std::sort(sortedSubmeshes.begin(), sortedSubmeshes.end());

        for (auto& submesh : sortedSubmeshes)
        {
            if (submesh.vertexCount == 0)
            {
                continue;
            }

            if (submesh.firstVertex < (range->firstVertex + range->vertexCount))
            {
                range->firstVertex = submesh.firstVertex + submesh.vertexCount;
            }

            if (submesh.firstIndex < (range->firstIndex + range->indexCount))
            {
                range->firstIndex = submesh.firstIndex + submesh.indexCount;
            }
        }
    }

    Mesh::Mesh() {}

    Mesh::Mesh(const BufferRef& vertexBuffer, const BufferRef& indexBuffer) : Mesh()
    {
        AddVertexBuffer(vertexBuffer);
        SetIndexBuffer(indexBuffer);
    }

    Mesh::Mesh(const BufferRef& vertexBuffer, const BufferRef& indexBuffer, const BoundingBox& bounds) : Mesh(vertexBuffer, indexBuffer)
    {
        m_fullRange = { 0u, (uint32_t)vertexBuffer->GetCount(), 0u, (uint32_t)indexBuffer->GetCount(), bounds };
    }

    void Mesh::Import(const char* filepath)
    {
        m_indexBuffer = nullptr;
        m_vertexBuffers.clear();
        m_submeshes.clear();

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

        m_freeSubmeshIndices.clear();
        m_fullRange = SubMesh();
        std::map<uint32_t, std::vector<BufferElement>> layoutMap;

        for (auto i = 0u; i < mesh->submeshCount; ++i)
        {
            auto bounds = BoundingBox::MinMax(Functions::ToFloat3(pSubmeshes[i].bbmin), Functions::ToFloat3(pSubmeshes[i].bbmax));
            m_submeshes.push_back({ 0u, mesh->vertexCount, pSubmeshes[i].firstIndex, pSubmeshes[i].indexCount, bounds });
            Functions::BoundsEncapsulate(&m_fullRange.bounds, bounds);
        }

        for (auto i = 0u; i < mesh->vertexAttributeCount; ++i)
        {
            layoutMap[pAttributes[i].stream].emplace_back(pAttributes[i].type, std::string(pAttributes[i].name));
        }

        auto pBufferOffset = 0ull;
        auto vertexBufferName = GetFileName() + std::string(".VertexBuffer");
        auto indexBufferName = GetFileName() + std::string(".IndexBuffer");
        auto cmd = GraphicsAPI::GetQueues()->GetCommandBuffer(QueueType::Transfer);

        for (auto& kv : layoutMap)
        {
            AddVertexBuffer(Buffer::Create(BufferLayout(kv.second), mesh->vertexCount, BufferUsage::DefaultVertex, vertexBufferName.c_str()));
            cmd->UploadBufferData(m_vertexBuffers.back().get(), (char*)pVertices + pBufferOffset);
            pBufferOffset += m_vertexBuffers.back()->GetLayout().GetStride() * mesh->vertexCount;
        }

        SetIndexBuffer(Buffer::Create(mesh->indexType, mesh->indexCount, BufferUsage::DefaultIndex, indexBufferName.c_str()));
        cmd->UploadBufferData(m_indexBuffer.get(), (char*)pVertices + pBufferOffset);
        m_uploadFence = cmd->GetFenceRef();

        PK::Assets::CloseAsset(&asset);
    }

    void Mesh::AllocateSubmeshRange(const SubmeshRangeAllocationInfo& allocationInfo, SubMesh* outAllocationRange, uint32_t* outSubmeshIndices)
    {
        SubMesh range = { 0u, allocationInfo.vertexCount, 0u, allocationInfo.indexCount, BoundingBox::GetMinBounds() };
        FindAllocationRange(m_submeshes, &range);

        for (auto i = 0u; i < allocationInfo.submeshCount; ++i)
        {
            if (m_freeSubmeshIndices.size() > 0)
            {
                outSubmeshIndices[i] = m_freeSubmeshIndices.back();
                m_freeSubmeshIndices.pop_back();
            }
            else
            {
                outSubmeshIndices[i] = (uint32_t)m_submeshes.size();
                m_submeshes.push_back({});
            }

            auto& submesh = m_submeshes[outSubmeshIndices[i]];
            submesh = allocationInfo.pSubmeshes[i];
            submesh.firstVertex += range.firstVertex;
            submesh.firstIndex += range.firstIndex;
            Math::Functions::BoundsEncapsulate(&m_fullRange.bounds, submesh.bounds);
            Math::Functions::BoundsEncapsulate(&range.bounds, submesh.bounds);
        }

        m_fullRange.vertexCount = glm::max(m_fullRange.vertexCount, range.firstVertex + range.vertexCount);
        m_fullRange.indexCount = glm::max(m_fullRange.indexCount, range.firstIndex + range.indexCount);

        AlignVertices((char*)allocationInfo.pVertices, allocationInfo.vertexCount, allocationInfo.vertexLayout, GetVertexBufferLayouts());

        auto cmd = GraphicsAPI::GetQueues()->GetCommandBuffer(QueueType::Transfer);
        auto pBufferOffset = 0ull;

        for (auto i = 0u; i < m_vertexBuffers.size(); ++i)
        {
            auto& vertexBuffer = m_vertexBuffers.at(i);
            PK_THROW_ASSERT(vertexBuffer->IsSparse(), "Cannot append vertices to a non sparse vertex buffer!");

            const auto& layout = vertexBuffer->GetLayout();
            auto vertexStride = layout.GetStride();

            vertexBuffer->MakeRangeResident({ range.firstVertex * vertexStride, range.vertexCount * vertexStride }, QueueType::Transfer);
            cmd->UploadBufferSubData(vertexBuffer.get(), (char*)allocationInfo.pVertices + pBufferOffset, range.firstVertex * vertexStride, range.vertexCount * vertexStride);
            pBufferOffset += vertexStride * range.vertexCount;
        }

        auto indexBuffer = m_indexBuffer.get();
        auto indexStride = m_indexBuffer->GetLayout().GetStride();
        PK_THROW_ASSERT(indexBuffer->IsSparse(), "Cannot append indices to a non sparse index buffer!");
        indexBuffer->MakeRangeResident({ range.firstIndex * indexStride, range.indexCount * indexStride }, QueueType::Transfer);

        // Convert 16bit indices to 32bit to avoid compatibility issues between meshes.
        if (ElementConvert::Size(allocationInfo.indexType) == 2)
        {
            auto view = cmd->BeginBufferWrite<uint32_t>(indexBuffer, range.firstIndex, range.indexCount);
            Functions::ReinterpretIndex16ToIndex32(view.data, reinterpret_cast<uint16_t*>(allocationInfo.pIndices), range.indexCount);
            cmd->EndBufferWrite(indexBuffer);
        }
        else
        {
            cmd->UploadBufferSubData(indexBuffer, allocationInfo.pIndices, range.firstIndex * indexStride, range.indexCount * indexStride);
        }

        m_uploadFence = cmd->GetFenceRef();
        *outAllocationRange = range;
    }

    void Mesh::DeallocateSubmeshRange(const SubMesh& allocationRange, uint32_t* submeshIndices, uint32_t submeshCount)
    {
        for (auto& vertexBuffer : m_vertexBuffers)
        {
            auto vertexStride = vertexBuffer->GetLayout().GetStride();
            PK_THROW_ASSERT(vertexBuffer->IsSparse(), "Trying to deallocate resources from a mesh without sparse buffers!");
            vertexBuffer->MakeRangeNonResident({ allocationRange.firstVertex * vertexStride, allocationRange.vertexCount * vertexStride });
        }

        auto indexBuffer = m_indexBuffer.get();
        auto indexStride = m_indexBuffer->GetLayout().GetStride();
        PK_THROW_ASSERT(indexBuffer->IsSparse(), "Trying to deallocate resources from a mesh without sparse buffers!");
        indexBuffer->MakeRangeNonResident({ allocationRange.firstIndex * indexStride, allocationRange.indexCount * indexStride });

        for (auto i = 0u; i < submeshCount; ++i)
        {
            m_freeSubmeshIndices.push_back(submeshIndices[i]);
            m_submeshes[i] = SubMesh();
        }
    }

    void Mesh::AddVertexBuffer(const BufferRef& vertexBuffer)
    {
        if (m_vertexBuffers.size() >= PK_MAX_VERTEX_ATTRIBUTES)
        {
            PK_LOG_WARNING("Warning! Trying to add more vertex buffers than supported!");
        }

        m_vertexBuffers.push_back(vertexBuffer);
        UpdatePositionAttributeInfo();
    }

    void Mesh::SetSubMeshes(const SubMesh* submeshes, size_t submeshCount)
    {
        auto count = submeshCount;
        m_fullRange = SubMesh();
        m_submeshes.resize(submeshCount);
        m_freeSubmeshIndices.clear();

        for (auto i = 0u; i < submeshCount; ++i)
        {
            m_submeshes[i] = submeshes[i];
            Functions::BoundsEncapsulate(&m_fullRange.bounds, submeshes[i].bounds);
            m_fullRange.vertexCount = glm::max(m_fullRange.vertexCount, submeshes[i].firstVertex + submeshes[i].vertexCount);
            m_fullRange.indexCount = glm::max(m_fullRange.indexCount, submeshes[i].firstIndex + submeshes[i].indexCount);
        }
    }

    void Mesh::UpdatePositionAttributeInfo()
    {
        auto vertexPositionHash = StringHashID::StringToID(PK_VS_POSITION);

        for (auto j = 0u; j < m_vertexBuffers.size(); ++j)
        {
            uint32_t positionIndex = 0u;
            auto vertexBuffer = m_vertexBuffers.at(j).get();
            auto vertexElement = vertexBuffer->GetLayout().TryGetElement(vertexPositionHash, &positionIndex);

            if (vertexElement != nullptr && vertexElement->Type == ElementType::Float3)
            {
                m_vertexPositionBufferIndex = j;
                m_vertexPositionOffset = vertexElement->Offset;
                return;
            }
        }
    }

    bool Mesh::TryGetAccelerationStructureGeometryInfo(uint32_t submesh, RHI::Objects::AccelerationStructureGeometryInfo* outInfo)
    {
        if (HasPendingUpload() || m_vertexPositionBufferIndex == ~0u)
        {
            return false;
        }

        auto& sm = GetSubmesh(submesh);
        outInfo->vertexBuffer = m_vertexBuffers.at(m_vertexPositionBufferIndex).get();
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

    const std::vector<const BufferLayout*> Mesh::GetVertexBufferLayouts() const
    {
        std::vector<const BufferLayout*> layouts;

        for (auto& vertexBuffer : m_vertexBuffers)
        {
            layouts.push_back(&vertexBuffer->GetLayout());
        }

        return layouts;
    }

    const SubMesh& Mesh::GetSubmesh(int32_t submesh) const
    {
        if (submesh < 0 || m_submeshes.empty())
        {
            return m_fullRange;
        }

        auto idx = glm::min((uint)submesh, (uint)m_submeshes.size());
        return m_submeshes.at(idx);
    }


    VirtualMesh::VirtualMesh()
    {
    }

    VirtualMesh::VirtualMesh(const SubmeshRangeAllocationInfo& data, Ref<Mesh> mesh)
    {
        m_mesh = mesh;
        m_submeshIndices.resize(data.submeshCount);
        m_mesh->AllocateSubmeshRange(data, &m_fullRange, m_submeshIndices.data());
    }

    VirtualMesh::~VirtualMesh()
    {
        if (m_mesh)
        {
            m_mesh->DeallocateSubmeshRange(m_fullRange, m_submeshIndices.data(), (uint32_t)m_submeshIndices.size());
        }
    }

    void VirtualMesh::Import(const char* filepath, Ref<Mesh>* pParams)
    {
        PK_THROW_ASSERT(pParams, "Cannot create a virtual mesh without a base mesh!");

        m_mesh = *pParams;

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

        m_submeshIndices.resize(mesh->submeshCount);

        SubmeshRangeAllocationInfo allocInfo{};
        allocInfo.pVertices = pVertices;
        allocInfo.pIndices = pIndices;
        allocInfo.vertexLayout = BufferLayout(bufferElements, false);
        allocInfo.pSubmeshes = submeshes.data();
        allocInfo.indexType = mesh->indexType;
        allocInfo.vertexCount = mesh->vertexCount;
        allocInfo.indexCount = mesh->indexCount;
        allocInfo.submeshCount = mesh->submeshCount;
        m_mesh->AllocateSubmeshRange(allocInfo, &m_fullRange, m_submeshIndices.data());

        PK::Assets::CloseAsset(&asset);
    }

    uint32_t VirtualMesh::GetSubmeshIndex(uint32_t submesh) const
    {
        auto idx = glm::min((uint32_t)submesh, (uint32_t)m_submeshIndices.size());
        return m_submeshIndices.at(idx);
    }
}

template<>
bool PK::Core::Services::AssetImporters::IsValidExtension<Mesh>(const std::filesystem::path& extension) { return extension.compare(".pkmesh") == 0; }

template<>
Ref<Mesh> PK::Core::Services::AssetImporters::Create()
{
    return CreateRef<Mesh>();
}

template<>
PK::Utilities::Ref<PK::Rendering::Objects::VirtualMesh> PK::Core::Services::AssetImporters::Create()
{
    return PK::Utilities::CreateRef<PK::Rendering::Objects::VirtualMesh>();
}

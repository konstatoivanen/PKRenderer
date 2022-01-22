#include "PrecompiledHeader.h"
#include "Mesh.h"
#include "Math/FunctionsIntersect.h"
#include "Math/FunctionsMisc.h"
#include <PKAssets/PKAssetLoader.h>

using namespace PK::Core;
using namespace PK::Utilities;
using namespace PK::Rendering;
using namespace PK::Rendering::Objects;

namespace PK::Rendering::Objects
{
    Mesh::Mesh()
    {
    }

    Mesh::Mesh(const Ref<Buffer>& vertexBuffer, const Ref<Buffer>& indexBuffer) : Mesh()
    {
        AddVertexBuffer(vertexBuffer);
        SetIndexBuffer(indexBuffer);
    }

	Mesh::Mesh(const Ref<Buffer>& vertexBuffer, const Ref<Buffer>& indexBuffer, const BoundingBox& bounds) : Mesh(vertexBuffer, indexBuffer)
	{
		m_fullRange = { 0u, (uint32_t)vertexBuffer->GetCount(), 0u, (uint32_t)indexBuffer->GetCount(), bounds };
	}

    void Mesh::Import(const char* filepath, void* pParams)
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
		std::vector<BufferElement> bufferElements;

		for (auto i = 0u; i < mesh->submeshCount; ++i)
		{
			auto bounds = BoundingBox::MinMax(Functions::ToFloat3(pSubmeshes[i].bbmin), Functions::ToFloat3(pSubmeshes[i].bbmax));
			m_submeshes.push_back({ 0u, mesh->vertexCount, pSubmeshes[i].firstIndex, pSubmeshes[i].indexCount, bounds });
			Functions::BoundsEncapsulate(&m_fullRange.bounds, bounds);
		}

		for (auto i = 0u; i < mesh->vertexAttributeCount; ++i)
		{
			bufferElements.emplace_back(pAttributes[i].type, std::string(pAttributes[i].name));
		}

		AddVertexBuffer(Buffer::CreateVertex(BufferLayout(bufferElements), pVertices, mesh->vertexCount));
		SetIndexBuffer(Buffer::CreateIndex(mesh->indexType, pIndexBuffer, mesh->indexCount));
        PK::Assets::CloseAsset(&asset);
    }

	void Mesh::AllocateSubmeshRange(const SubmeshRangeAllocationInfo& allocationInfo, 
									SubMesh* outAllocationRange,
									uint32_t* outSubmeshIndices)
	{
		const auto& layout = GetDefaultLayout();
		auto vbuff = m_vertexBuffers.at(0).get();
		auto ibuff = m_indexBuffer.get();
		auto vstride = GetDefaultLayout().GetStride();
		auto istride = m_indexBuffer->GetLayout().GetStride();

		if (!vbuff->IsSparse() ||
			!ibuff->IsSparse())
		{
			PK_THROW_ERROR("Trying to append resources to a mesh without sparse buffers!");
		}

		PK_THROW_ASSERT(allocationInfo.vertexLayout.size()  == layout.size(), "Vertex attribute count missmatch!");

		for (auto i = 0u; i < layout.size(); ++i)
		{
			PK_THROW_ASSERT(allocationInfo.vertexLayout.at(i) == layout.at(i), "Vertex attribute type missmatch!");
		}

		SubMesh range = { 0u, allocationInfo.vertexCount, 0u, allocationInfo.indexCount, BoundingBox::GetMinBounds() };

		// @TODO refactor this to be more memory efficient
		auto sortedSubmeshes = std::vector<SubMesh>(m_submeshes);
		std::sort(sortedSubmeshes.begin(), sortedSubmeshes.end());

		for (auto& sm : sortedSubmeshes)
		{
			if (sm.vertexCount == 0)
			{
				continue;
			}

			if (sm.firstVertex < (range.firstVertex + range.vertexCount))
			{
				range.firstVertex = sm.firstVertex + sm.vertexCount;
			}

			if (sm.firstIndex < (range.firstIndex + range.indexCount))
			{
				range.firstIndex = sm.firstIndex + sm.indexCount;
			}
		}

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

			auto& sm = m_submeshes[outSubmeshIndices[i]];
			sm = allocationInfo.pSubmeshes[i];
			sm.firstVertex += range.firstVertex;
			sm.firstIndex += range.firstIndex;
			Math::Functions::BoundsEncapsulate(&m_fullRange.bounds, sm.bounds);
			Math::Functions::BoundsEncapsulate(&range.bounds, sm.bounds);

		}

		m_fullRange.vertexCount = glm::max(m_fullRange.vertexCount, range.firstVertex + range.vertexCount);
		m_fullRange.indexCount = glm::max(m_fullRange.indexCount, range.firstIndex + range.indexCount);

		vbuff->MakeRangeResident({ range.firstVertex * vstride, range.vertexCount * vstride });
		ibuff->MakeRangeResident({ range.firstIndex * istride, range.indexCount * istride });

		vbuff->SetSubData(allocationInfo.pVertices, range.firstVertex * vstride, range.vertexCount * vstride);

		// Convert 16bit indices to 32bit to avoid compatibility issues between meshes.
		if (ElementConvert::Size(allocationInfo.indexType) == 2)
		{
			Functions::ReinterpretIndex16ToIndex32(ibuff->BeginMap<uint32_t>(range.firstIndex, range.indexCount).data, 
												   reinterpret_cast<uint16_t*>(allocationInfo.pIndices), range.indexCount);
			ibuff->EndMap();
		}
		else
		{
			ibuff->SetSubData(allocationInfo.pIndices, range.firstIndex * istride, range.indexCount * istride);
		}
		
		*outAllocationRange = range;
	}

	void Mesh::DeallocateSubmeshRange(const SubMesh& allocationRange, uint32_t* submeshIndices, uint32_t submeshCount)
	{
		auto vbuff = m_vertexBuffers.at(0).get();
		auto ibuff = m_indexBuffer.get();

		if ((vbuff->GetUsage() & BufferUsage::Sparse) == 0 ||
			(ibuff->GetUsage() & BufferUsage::Sparse) == 0)
		{
			PK_THROW_ERROR("Trying to deallocate resources from a mesh without sparse buffers!");
		}

		auto vstride = GetDefaultLayout().GetStride();
		auto istride = m_indexBuffer->GetLayout().GetStride();

		vbuff->MakeRangeNonResident({ allocationRange.firstVertex * vstride, allocationRange.vertexCount * vstride });
		ibuff->MakeRangeNonResident({ allocationRange.firstIndex * istride, allocationRange.indexCount * istride });

		for (auto i = 0u; i < submeshCount; ++i)
		{
			m_freeSubmeshIndices.push_back(submeshIndices[i]);
			m_submeshes[i] = SubMesh();
		}
	}

	void Mesh::AddVertexBuffer(const Ref<Buffer>& vertexBuffer)
    {
        if (m_vertexBuffers.size() >= PK_MAX_VERTEX_ATTRIBUTES)
        {
            PK_LOG_WARNING("Warning! Trying to add more vertex buffers than supported!");
        }

        m_vertexBuffers.push_back(vertexBuffer);
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

    const SubMesh& Mesh::GetSubmesh(int submesh) const
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
bool AssetImporters::IsValidExtension<Mesh>(const std::filesystem::path& extension) { return extension.compare(".ktx2") == 0; }

template<>
Ref<Mesh> AssetImporters::Create()
{
    return CreateRef<Mesh>();
}

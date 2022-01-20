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
		m_fullBounds = bounds;
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

		m_fullBounds = BoundingBox::GetMinBounds();
		std::vector<BufferElement> bufferElements;

		for (auto i = 0u; i < mesh->submeshCount; ++i)
		{
			m_submeshes.push_back({ 0u, mesh->vertexCount, pSubmeshes[i].firstIndex, pSubmeshes[i].indexCount });
			m_boundingBoxes.push_back({});
			auto& b = m_boundingBoxes.at(m_boundingBoxes.size() - 1);
			memcpy(glm::value_ptr(b.min), pSubmeshes[i].bbmin, sizeof(float) * 3);
			memcpy(glm::value_ptr(b.max), pSubmeshes[i].bbmax, sizeof(float) * 3);
			Functions::BoundsEncapsulate(&m_fullBounds, b);
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

		if ((vbuff->GetUsage() & BufferUsage::Sparse) == 0 ||
			(ibuff->GetUsage() & BufferUsage::Sparse) == 0)
		{
			PK_THROW_ERROR("Trying to append resources to a mesh without sparse buffers!");
		}

		PK_THROW_ASSERT(allocationInfo.vertexLayout.size()  == layout.size(), "Vertex attribute count missmatch!");

		for (auto i = 0u; i < layout.size(); ++i)
		{
			PK_THROW_ASSERT(allocationInfo.vertexLayout.at(i) == layout.at(i), "Vertex attribute type missmatch!");
		}

		SubMesh range = { 0u, allocationInfo.vertexCount, 0u, allocationInfo.indexCount };

		// @TODO refactor this to be more memory efficient
		auto sortedSubmeshes = std::vector<SubMesh>(m_submeshes);
		std::sort(sortedSubmeshes.begin(), sortedSubmeshes.end());

		for (auto& sm : sortedSubmeshes)
		{
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
			outSubmeshIndices[i] = (uint32_t)m_submeshes.size();
			auto sm = allocationInfo.pSubmeshes[i];
			auto b = allocationInfo.pBoundingBoxes + i;
			sm.firstVertex += range.firstVertex;
			sm.firstIndex += range.firstIndex;
			m_submeshes.push_back(sm);
			m_boundingBoxes.push_back(*b);
			Math::Functions::BoundsEncapsulate(&m_fullBounds, *b);
		}

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

	void Mesh::DeallocateSubmeshRange(const SubMesh& allocationRange)
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

		//@TODO remove submeshes
	}

	void Mesh::AddVertexBuffer(const Ref<Buffer>& vertexBuffer)
    {
        if (m_vertexBuffers.size() >= PK_MAX_VERTEX_ATTRIBUTES)
        {
            PK_LOG_WARNING("Warning! Trying to add more vertex buffers than supported!");
        }

        m_vertexBuffers.push_back(vertexBuffer);
    }

	void Mesh::SetSubMeshes(const SubMesh* submeshes, const BoundingBox* boundingBoxes, size_t submeshCount, size_t boundCount)
	{
		PK_THROW_ASSERT(submeshCount == boundCount, "Submesh & Bounding box array size missmatch!");
	
		auto count = submeshCount;
		m_fullBounds = BoundingBox::GetMinBounds();
		m_submeshes.resize(submeshCount);
		m_boundingBoxes.resize(submeshCount);

		for (auto i = 0u; i < submeshCount; ++i)
		{
			m_submeshes[i] = submeshes[i];
			m_boundingBoxes[i] = boundingBoxes[i];
			auto b = boundingBoxes + i;

			for (auto k = 0; k < 3; ++k)
			{
				if (m_fullBounds.max[k] < b->max[k])
				{
					m_fullBounds.max[k] = b->max[k];
				}

				if (m_fullBounds.min[k] > b->min[k])
				{
					m_fullBounds.min[k] = b->min[k];
				}
			}
		}
	}

    const SubMesh Mesh::GetSubmesh(int submesh) const
    {
        if (submesh < 0 || m_submeshes.empty())
        {
            return { 0u, (uint32_t)m_vertexBuffers.at(0)->GetCount(), 0u, (uint)m_indexBuffer->GetCount() };
        }

        auto idx = glm::min((uint)submesh, (uint)m_submeshes.size());
        return m_submeshes.at(idx);
    }

	const BoundingBox& Mesh::GetBounds(int submesh) const
	{
		if (submesh < 0 || m_submeshes.empty())
		{
			return m_fullBounds;
		}

		auto idx = glm::min((uint)submesh, (uint)m_submeshes.size());
		return m_boundingBoxes.at(idx);
	}
}

template<>
bool AssetImporters::IsValidExtension<Mesh>(const std::filesystem::path& extension) { return extension.compare(".ktx2") == 0; }

template<>
Ref<Mesh> AssetImporters::Create()
{
    return CreateRef<Mesh>();
}

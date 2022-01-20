#include "PrecompiledHeader.h"
#include "VirtualMesh.h"
#include <PKAssets/PKAssetLoader.h>

using namespace PK::Core;
using namespace PK::Utilities;
using namespace PK::Rendering;
using namespace PK::Rendering::Objects;

namespace PK::Rendering::Objects
{
    VirtualMesh::VirtualMesh()
    {
    }
    
    VirtualMesh::~VirtualMesh()
    {
		if (m_mesh)
		{
			m_mesh->DeallocateSubmeshRange(m_fullRange);
		}
    }
    
    void VirtualMesh::Import(const char* filepath, void* pParams)
    {
        PK_THROW_ASSERT(pParams, "Cannot create a virtual mesh without a base mesh!");

        //@TODO maybe replace this with a import specialization template? less volatile if we can keep this type safe.
        m_mesh = *reinterpret_cast<Ref<Mesh>*>(pParams);

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
		std::vector<BoundingBox> bounds;
		submeshes.reserve(mesh->submeshCount);
		bounds.reserve(mesh->submeshCount);

		for (auto i = 0u; i < mesh->submeshCount; ++i)
		{
			submeshes.push_back({ 0u, mesh->vertexCount, pSubmeshes[i].firstIndex, pSubmeshes[i].indexCount });
			bounds.push_back({});
			auto& b = bounds.at(bounds.size() - 1);
			memcpy(glm::value_ptr(b.min), pSubmeshes[i].bbmin, sizeof(float) * 3);
			memcpy(glm::value_ptr(b.max), pSubmeshes[i].bbmax, sizeof(float) * 3);
		}

		for (auto i = 0u; i < mesh->vertexAttributeCount; ++i)
		{
			bufferElements.emplace_back(pAttributes[i].type, std::string(pAttributes[i].name));
		}

		m_submeshIndices.resize(mesh->submeshCount);

		SubmeshRangeAllocationInfo allocInfo{};
		allocInfo.pVertices = pVertices;
		allocInfo.pIndices = pIndices;
		allocInfo.vertexLayout = BufferLayout(bufferElements);
		allocInfo.pSubmeshes = submeshes.data();
		allocInfo.pBoundingBoxes = bounds.data();
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
Ref<VirtualMesh> AssetImporters::Create()
{
	return CreateRef<VirtualMesh>();
}
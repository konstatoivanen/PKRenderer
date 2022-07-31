#include "PrecompiledHeader.h"
#include "VirtualMesh.h"
#include <PKAssets/PKAssetLoader.h>
#include "Math/FunctionsMisc.h"
#include "Math/FunctionsIntersect.h"

namespace PK::Rendering::Objects
{
	using namespace Core;
	using namespace Math;
	using namespace Utilities;
	using namespace Structs;

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
PK::Utilities::Ref<PK::Rendering::Objects::VirtualMesh> PK::Core::Services::AssetImporters::Create()
{
	return PK::Utilities::CreateRef<PK::Rendering::Objects::VirtualMesh>();
}
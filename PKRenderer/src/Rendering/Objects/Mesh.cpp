#include "PrecompiledHeader.h"
#include "Mesh.h"
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

	Mesh::Mesh(const Ref<Buffer>& vertexBuffer, const Ref<Buffer>& indexBuffer, const BoundingBox& localBounds) : Mesh(vertexBuffer, indexBuffer)
	{
		m_localBounds = localBounds;
	}

    void Mesh::Import(const char* filepath)
    {
		m_indexBuffer = nullptr;
		m_vertexBuffers.clear();
		m_indexRanges.clear();

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

		std::vector<BufferElement> bufferElements;

		for (auto i = 0u; i < mesh->submeshCount; ++i)
		{
			m_indexRanges.push_back({ pSubmeshes[i].offset, pSubmeshes[i].count });
		}

		for (auto i = 0u; i < mesh->vertexAttributeCount; ++i)
		{
			bufferElements.emplace_back(pAttributes[i].type, std::string(pAttributes[i].name));
		}

		AddVertexBuffer(Buffer::CreateVertex(BufferLayout(bufferElements), pVertices, mesh->vertexCount));
		SetIndexBuffer(Buffer::CreateIndex(mesh->indexType, pIndexBuffer, mesh->indexCount));

        PK::Assets::CloseAsset(&asset);
    }

    void Mesh::AddVertexBuffer(const Ref<Buffer>& vertexBuffer)
    {
        if (m_vertexBuffers.size() >= PK_MAX_VERTEX_ATTRIBUTES)
        {
            PK_LOG_WARNING("Warning! Trying to add more vertex buffers than supported!");
        }

        m_vertexBuffers.push_back(vertexBuffer);
    }

    const IndexRange Mesh::GetSubmesh(int submesh) const
    {
        if (submesh < 0 || m_indexRanges.empty())
        {
            return { 0, (uint)m_indexBuffer->GetCount() };
        }

        auto idx = glm::min((uint)submesh, (uint)m_indexRanges.size());
        return m_indexRanges.at(idx);
    }
}

template<>
bool AssetImporters::IsValidExtension<Mesh>(const std::filesystem::path& extension) { return extension.compare(".ktx2") == 0; }

template<>
Ref<Mesh> AssetImporters::Create()
{
    return CreateRef<Mesh>();
}

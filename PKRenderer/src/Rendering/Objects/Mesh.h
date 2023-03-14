#pragma once
#include "Core/Services/AssetDatabase.h"
#include "Rendering/Objects/Buffer.h"
#include "Rendering/Structs/StructsCommon.h"
#include "Rendering/Structs/FenceRef.h"

namespace PK::Rendering::Objects
{
    struct SubMesh
    {
        uint32_t firstVertex = 0u;
        uint32_t vertexCount = 0u;
        uint32_t firstIndex = 0u;
        uint32_t indexCount = 0u;
        Math::BoundingBox bounds = Math::BoundingBox::GetMinBounds();

        constexpr bool operator < (const SubMesh& other)
        {
            if (firstVertex != other.firstVertex) return firstVertex < other.firstVertex;
            if (firstIndex != other.firstIndex) return firstIndex < other.firstIndex;
            if (vertexCount != other.vertexCount) return vertexCount < other.vertexCount;
            if (indexCount != other.indexCount) return indexCount < other.indexCount;
            return false;
        }
    };

    struct SubmeshRangeAllocationInfo
    {
        void* pVertices;
        void* pIndices;
        Structs::BufferLayout vertexLayout;
        SubMesh* pSubmeshes;
        Structs::ElementType indexType;
        uint32_t vertexCount;
        uint32_t indexCount;
        uint32_t submeshCount;
    };

    class Mesh : public Core::Services::Asset, public Core::Services::IAssetImportSimple
    {
        friend Utilities::Ref<Mesh> Core::Services::AssetImporters::Create();

        public:
            Mesh();
            Mesh(const Utilities::Ref<Buffer>& vertexBuffer, const Utilities::Ref<Buffer>& indexBuffer);
            Mesh(const Utilities::Ref<Buffer>& vertexBuffer, const Utilities::Ref<Buffer>& indexBuffer, const Math::BoundingBox& bounds);

            void Import(const char* filepath) override final;

            /// Allocates a submesh range by appending submitted data to the first assigned vertex buffer.
            /// - Requires the first vertex buffer to be virtual.
            /// - Validates data interleaving using the submitted layout, throws an exception upon missmatch.
            void AllocateSubmeshRange(const SubmeshRangeAllocationInfo& allocationInfo,
                                      SubMesh* outAllocationRange,
                                      uint32_t* outSubmeshIndices);

            void DeallocateSubmeshRange(const SubMesh& allocationRange, uint32_t* submeshIndices, uint32_t submeshCount);

            void AddVertexBuffer(const Utilities::Ref<Buffer>& vertexBuffer);
            void SetIndexBuffer(const Utilities::Ref<Buffer>& indexBuffer) { m_indexBuffer = indexBuffer; }
            void SetSubMeshes(const SubMesh* submeshes, size_t submeshCount);
            inline void SetSubMeshes(const std::initializer_list<SubMesh>& submeshes) { SetSubMeshes(submeshes.begin(), submeshes.end() - submeshes.begin()); }
            inline void SetSubMeshes(const std::vector<SubMesh>& submeshes) { SetSubMeshes(submeshes.data(), submeshes.size()); }

            constexpr const std::vector<Utilities::Ref<Buffer>>& GetVertexBuffers() const { return m_vertexBuffers; }
            const std::vector<const Structs::BufferLayout*> GetVertexBufferLayouts() const;
            const Buffer* GetVertexBuffer(uint32_t index) const { return m_vertexBuffers.at(index).get(); }
            const Buffer* GetIndexBuffer() const { return m_indexBuffer.get(); }
            const SubMesh& GetSubmesh(int32_t submesh) const;
            inline const uint32_t GetSubmeshCount() const { return glm::max(1u, (uint32_t)m_submeshes.size() - (uint32_t)m_freeSubmeshIndices.size()); }
            constexpr const SubMesh& GetFullRange() const { return m_fullRange; }
            const bool HasPendingUpload() const { return !m_uploadFence.WaitInvalidate(0ull); }

        private:
            std::vector<Utilities::Ref<Buffer>> m_vertexBuffers;
            Utilities::Ref<Buffer> m_indexBuffer;
            SubMesh m_fullRange{};
            mutable Structs::FenceRef m_uploadFence;

            std::vector<SubMesh> m_submeshes;
            std::vector<uint32_t> m_freeSubmeshIndices;
    };
}
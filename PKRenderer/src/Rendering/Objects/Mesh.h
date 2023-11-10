#pragma once
#include "Core/Services/AssetDatabase.h"
#include "Rendering/RHI/GraphicsAPI.h"

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
        RHI::BufferLayout vertexLayout;
        SubMesh* pSubmeshes;
        RHI::ElementType indexType;
        uint32_t vertexCount;
        uint32_t indexCount;
        uint32_t submeshCount;
    };

    class Mesh : public Core::Services::Asset, public Core::Services::IAssetImportSimple
    {
        friend Utilities::Ref<Mesh> Core::Services::AssetImporters::Create();

        public:
            Mesh();
            Mesh(const RHI::Objects::BufferRef& vertexBuffer, const RHI::Objects::BufferRef& indexBuffer);
            Mesh(const RHI::Objects::BufferRef& vertexBuffer, const RHI::Objects::BufferRef& indexBuffer, const Math::BoundingBox& bounds);

            void Import(const char* filepath) final;

            /// Allocates a submesh range by appending submitted data to the first assigned vertex buffer.
            /// - Requires the first vertex buffer to be virtual.
            /// - Validates data interleaving using the submitted layout, throws an exception upon missmatch.
            void AllocateSubmeshRange(const SubmeshRangeAllocationInfo& allocationInfo,
                                      SubMesh* outAllocationRange,
                                      uint32_t* outSubmeshIndices);

            void DeallocateSubmeshRange(const SubMesh& allocationRange, uint32_t* submeshIndices, uint32_t submeshCount);

            void AddVertexBuffer(const RHI::Objects::BufferRef& vertexBuffer);
            void SetIndexBuffer(const RHI::Objects::BufferRef& indexBuffer) { m_indexBuffer = indexBuffer; }
            void SetSubMeshes(const SubMesh* submeshes, size_t submeshCount);
            void UpdatePositionAttributeInfo();
            bool TryGetAccelerationStructureGeometryInfo(uint32_t submesh, RHI::Objects::AccelerationStructureGeometryInfo* outInfo);
            inline void SetSubMeshes(const std::initializer_list<SubMesh>& submeshes) { SetSubMeshes(submeshes.begin(), submeshes.end() - submeshes.begin()); }
            inline void SetSubMeshes(const std::vector<SubMesh>& submeshes) { SetSubMeshes(submeshes.data(), submeshes.size()); }

            constexpr const std::vector<RHI::Objects::BufferRef>& GetVertexBuffers() const { return m_vertexBuffers; }
            const std::vector<const RHI::BufferLayout*> GetVertexBufferLayouts() const;
            const RHI::Objects::Buffer* GetVertexBuffer(uint32_t index) const { return m_vertexBuffers.at(index).get(); }
            const RHI::Objects::Buffer* GetIndexBuffer() const { return m_indexBuffer.get(); }
            const SubMesh& GetSubmesh(int32_t submesh) const;
            inline const uint32_t GetSubmeshCount() const { return glm::max(1u, (uint32_t)m_submeshes.size() - (uint32_t)m_freeSubmeshIndices.size()); }
            constexpr const SubMesh& GetFullRange() const { return m_fullRange; }
            const bool HasPendingUpload() const { return !m_uploadFence.WaitInvalidate(0ull); }


        private:
            std::vector<RHI::Objects::BufferRef> m_vertexBuffers;
            RHI::Objects::BufferRef m_indexBuffer;
            SubMesh m_fullRange{};
            mutable RHI::FenceRef m_uploadFence;

            // Cached for blas geometry infos
            uint32_t m_vertexPositionBufferIndex = ~0u;
            uint32_t m_vertexPositionOffset = ~0u;
            std::vector<SubMesh> m_submeshes;
            std::vector<uint32_t> m_freeSubmeshIndices;
    };

    class VirtualMesh : public Core::Services::Asset, public Core::Services::IAssetImport<Utilities::Ref<Mesh>*>
    {
        friend Utilities::Ref<VirtualMesh> Core::Services::AssetImporters::Create();

        public:
            VirtualMesh();
            VirtualMesh(const SubmeshRangeAllocationInfo& data, Utilities::Ref<Mesh> mesh);
            ~VirtualMesh();

            virtual void Import(const char* filepath, Utilities::Ref<Mesh>* pParams) final;
            inline Mesh* GetBaseMesh() const { return m_mesh.get(); }
            uint32_t GetSubmeshIndex(uint32_t submesh) const;
            uint32_t GetBaseSubmeshIndex() const { return m_submeshIndices.at(0); }
            inline const uint32_t GetSubmeshCount() const { return glm::max(1, (int)m_submeshIndices.size()); }

        private:
            Utilities::Ref<Mesh> m_mesh = nullptr;
            SubMesh m_fullRange{};
            std::vector<uint32_t> m_submeshIndices;
    };
}
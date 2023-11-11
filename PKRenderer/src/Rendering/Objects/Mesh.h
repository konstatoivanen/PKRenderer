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
    };

    struct SubmeshAllocation
    {
        uint32_t firstVertex = 0u;
        uint32_t vertexCount = 0u;
        uint32_t firstIndex = 0u;
        uint32_t indexCount = 0u;
        // @TODO dont like this dynamic stuff
        // Submesh handling for regular meshes in general is bad.
        // Consider refactoring. Maybe some constant limit?
        std::vector<uint32_t> submeshIndices;
    };

    struct SubmeshRangeData
    {
        void* pVertices;
        void* pIndices;
        SubMesh* pSubmeshes;
        RHI::BufferLayout vertexLayout;
        RHI::ElementType indexType;
        uint32_t vertexCount;
        uint32_t indexCount;
        uint32_t submeshCount;
    };

    class Mesh : public Core::Services::Asset, public Core::Services::IAssetImportSimple
    {
        friend Utilities::Ref<Mesh> Core::Services::AssetImporters::Create();

        public:
            typedef Utilities::FixedList<RHI::Objects::BufferRef, PK::Rendering::RHI::PK_MAX_VERTEX_ATTRIBUTES> VertexBuffers;

            Mesh();
            Mesh(const RHI::Objects::BufferRef& indexBuffer, RHI::Objects::BufferRef* vertexBuffers, uint32_t vertexBufferCount, SubMesh* submeshes, uint32_t submeshCount);
            
            void SetResources(const RHI::Objects::BufferRef& indexBuffer, RHI::Objects::BufferRef* vertexBuffers, uint32_t vertexBufferCount, SubMesh* submeshes, uint32_t submeshCount);

            void AlignVertices(char* vertices, size_t vcount, const RHI::BufferLayout& layout);

            void Import(const char* filepath) final;

            /// Allocates a submesh range by appending submitted data to the first assigned vertex buffer.
            /// - Requires the vertex buffers to be virtual.
            /// - Validates data interleaving using the submitted layout, throws an exception upon missmatch.
            SubmeshAllocation AllocateSubmeshRange(const SubmeshRangeData* data);
            void DeallocateSubmeshRange(const SubmeshAllocation& allocation);
            
            bool TryGetAccelerationStructureGeometryInfo(uint32_t submesh, RHI::Objects::AccelerationStructureGeometryInfo* outInfo);

            constexpr const VertexBuffers& GetVertexBuffers() const { return m_vertexBuffers; }
            const RHI::Objects::Buffer* GetVertexBuffer(uint32_t index) const { return m_vertexBuffers[index].get(); }
            const RHI::Objects::Buffer* GetIndexBuffer() const { return m_indexBuffer.get(); }
            const SubMesh& GetSubmesh(int32_t submesh) const;
            inline const uint32_t GetSubmeshCount() const { return glm::max(1u, (uint32_t)m_submeshes.size() - (uint32_t)m_freeSubmeshIndices.size()); }
            constexpr const SubMesh& GetFullRange() const { return m_fullRange; }
            const bool HasPendingUpload() const { return !m_uploadFence.WaitInvalidate(0ull); }

        private:
            VertexBuffers m_vertexBuffers;
            RHI::Objects::BufferRef m_indexBuffer;
            mutable RHI::FenceRef m_uploadFence;

            // Cached for blas geometry infos & general convenience.
            SubMesh m_fullRange{};
            uint32_t m_vertexPositionBufferIndex = ~0u;
            uint32_t m_vertexPositionOffset = ~0u;
            std::vector<SubMesh> m_submeshes;
            std::vector<uint32_t> m_freeSubmeshIndices;
    };
}
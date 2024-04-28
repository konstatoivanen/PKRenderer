#pragma once
#include "Utilities/FixedList.h"
#include "Core/Assets/Asset.h"
#include "Rendering/RHI/FenceRef.h"
#include "Rendering/RHI/Objects/Buffer.h"
#include "Rendering/RHI/Objects/AccelerationStructure.h"

namespace PK::Rendering::Objects
{
    class Mesh : public Core::Assets::AssetWithImport<>
    {
        public:
            struct SubMesh
            {
                uint32_t firstVertex = 0u;
                uint32_t vertexCount = 0u;
                uint32_t firstIndex = 0u;
                uint32_t indexCount = 0u;
                Math::BoundingBox bounds = Math::BoundingBox::GetMinBounds();
            };

            typedef Utilities::FixedList<RHI::Objects::BufferRef, PK::Rendering::RHI::PK_MAX_VERTEX_ATTRIBUTES> VertexBuffers;

            Mesh();
            Mesh(const RHI::Objects::BufferRef& indexBuffer, RHI::Objects::BufferRef* vertexBuffers, uint32_t vertexBufferCount, SubMesh* submeshes, uint32_t submeshCount);
            void SetResources(const RHI::Objects::BufferRef& indexBuffer, RHI::Objects::BufferRef* vertexBuffers, uint32_t vertexBufferCount, SubMesh* submeshes, uint32_t submeshCount);
            void AssetImport(const char* filepath) final;
            bool TryGetAccelerationStructureGeometryInfo(uint32_t submesh, RHI::Objects::AccelerationStructureGeometryInfo* outInfo);

            constexpr const VertexBuffers& GetVertexBuffers() const { return m_vertexBuffers; }
            const RHI::Objects::Buffer* GetVertexBuffer(uint32_t index) const { return m_vertexBuffers[index].get(); }
            const RHI::Objects::Buffer* GetIndexBuffer() const { return m_indexBuffer.get(); }
            const SubMesh& GetSubmesh(int32_t submesh) const;
            inline const uint32_t GetSubmeshCount() const { return glm::max(1u, (uint32_t)m_submeshes.size()); }
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
    };
}
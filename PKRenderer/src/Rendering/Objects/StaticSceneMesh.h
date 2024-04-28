#pragma once
#include "Utilities/FixedPool.h"
#include "Rendering/RHI/FenceRef.h"
#include "Rendering/RHI/Objects/Buffer.h"
#include "Rendering/RHI/Objects/AccelerationStructure.h"

namespace PK::Rendering::Objects
{
    class StaticSceneMesh;

    struct StaticMeshAllocationData
    {
        struct SubMesh
        {
            uint32_t vertexFirst = 0u;
            uint32_t vertexCount = 0u;
            uint32_t indexFirst = 0u;
            uint32_t indexCount = 0u;
            Math::BoundingBox bounds = Math::BoundingBox::GetMinBounds();
        };

        uint32_t nameHashId = 0u;

        struct Regular
        {
            void* pVertices;
            void* pIndices;
            SubMesh* pSubmeshes;
            RHI::BufferLayout vertexLayout;
            RHI::ElementType indexType;
            uint32_t vertexCount;
            uint32_t indexCount;
            uint32_t submeshCount;
        } 
        regular;

        struct Meshlet
        {
            Assets::Mesh::Meshlet::PKSubmesh* pSubmeshes;
            uint32_t submeshCount;
            Assets::Mesh::Meshlet::PKMeshlet* pMeshlets;
            uint32_t meshletCount;
            Assets::Mesh::Meshlet::PKVertex* pVertices;
            uint32_t vertexCount;
            uint8_t* pIndices;
            uint32_t triangleCount;
        } 
        meshlet;
    };

    struct StaticSubMesh
    {
        uint32_t nameHashId = 0u;
        uint32_t meshletFirst = 0u;
        uint32_t meshletCount = 0u;
        uint32_t vertexFirst = 0u;
        uint32_t vertexCount = 0u;
        uint32_t indexFirst = 0u;
        uint32_t indexCount = 0u;
        Math::BoundingBox bounds = Math::BoundingBox::GetMinBounds();
    };

    struct StaticMesh
    {
        StaticSceneMesh* baseMesh = nullptr;
        uint32_t nameHashId = 0u;
        uint32_t submeshFirst = 0u;
        uint32_t submeshCount = 0u;
        uint32_t meshletFirst = 0u;
        uint32_t meshletCount = 0u;
        uint32_t meshletVertexFirst = 0u;
        uint32_t meshletVertexCount = 0u;
        uint32_t meshletTriangleFirst = 0u;
        uint32_t meshletTriangleCount = 0u;
        uint32_t vertexFirst = 0u;
        uint32_t vertexCount = 0u;
        uint32_t indexFirst = 0u;
        uint32_t indexCount = 0u;

        inline uint32_t GetGlobalSubmeshIndex(uint32_t localIndex) const { return submeshFirst + glm::min(localIndex, submeshCount - 1u); }
        const StaticSubMesh* GetSubmesh(uint32_t localIndex) const;
        bool TryGetAccelerationStructureGeometryInfo(uint32_t localIndex, RHI::Objects::AccelerationStructureGeometryInfo* outInfo) const;
    };

    class StaticSceneMesh : public PK::Utilities::NoCopy
    {
        public:
            StaticSceneMesh();
            inline RHI::Objects::Buffer* GetPositionBuffer() const { return m_positionsBuffer.get(); }
            inline RHI::Objects::Buffer* GetAttributeBuffer() const { return m_attributesBuffer.get(); }
            inline RHI::Objects::Buffer* GetIndexBuffer() const { return m_indexBuffer.get(); }
            inline RHI::Objects::Buffer* GetMeshletVertexBuffer() const { return m_meshletVertexBuffer.get(); }
            inline RHI::Objects::Buffer* GetMeshletIndexBuffer() const { return m_meshletIndexBuffer.get(); }
            inline RHI::Objects::Buffer* GetMeshletSubmeshBuffer() const { return m_submeshBuffer.get(); }
            inline RHI::Objects::Buffer* GetMeshletBuffer() const { return m_meshletBuffer.get(); }
            inline const StaticSubMesh* GetSubmesh(uint32_t index) const { return m_staticSubmeshes[index]; }
            const bool HasPendingUpload() const { return !m_uploadFence.WaitInvalidate(0ull); }
            
            StaticMesh* Allocate(StaticMeshAllocationData* data);
            void Deallocate(StaticMesh* mesh);

            bool TryGetAccelerationStructureGeometryInfo(uint32_t globalSubmeshIndex, RHI::Objects::AccelerationStructureGeometryInfo* outInfo) const;

        private:
            void AlignVertices(char* vertices, size_t vcount, const RHI::BufferLayout& layout);

            Utilities::FixedPool<StaticMesh, 4096> m_staticMeshes;
            Utilities::FixedPool<StaticSubMesh, 8192> m_staticSubmeshes;

            uint32_t m_submeshCount = 0u;
            uint32_t m_meshletCount = 0u;
            uint32_t m_meshletVertexCount = 0u;
            uint32_t m_meshletTriangleCount = 0u;
            uint32_t m_vertexCount = 0u;
            uint32_t m_indexCount = 0u;

            RHI::Objects::BufferRef m_positionsBuffer;
            RHI::Objects::BufferRef m_attributesBuffer;
            RHI::Objects::BufferRef m_indexBuffer;

            RHI::Objects::BufferRef m_submeshBuffer;
            RHI::Objects::BufferRef m_meshletBuffer;
            RHI::Objects::BufferRef m_meshletVertexBuffer;
            RHI::Objects::BufferRef m_meshletIndexBuffer;
            mutable RHI::FenceRef m_uploadFence;
    };
}
#pragma once
#include "Utilities/FixedPool.h"
#include "Utilities/FenceRef.h"
#include "Graphics/RHI/Layout.h"
#include "Graphics/GraphicsFwd.h"

namespace PK::Graphics
{
    class MeshStaticCollection;

    struct MeshStaticAllocationData
    {
        struct SubMesh
        {
            uint32_t vertexFirst = 0u;
            uint32_t vertexCount = 0u;
            uint32_t indexFirst = 0u;
            uint32_t indexCount = 0u;
            Math::BoundingBox bounds = Math::BoundingBox::GetMinBounds();
        };

        Utilities::NameID name = 0u;

        struct Regular
        {
            void* pVertices;
            void* pIndices;
            SubMesh* pSubmeshes;
            RHI::VertexStreamLayout streamLayout;
            RHI::ElementType indexType;
            uint32_t vertexCount;
            uint32_t indexCount;
            uint32_t submeshCount;
        } 
        regular;

        struct Meshlet
        {
            PK::Assets::Mesh::Meshlet::PKSubmesh* pSubmeshes;
            uint32_t submeshCount;
            PK::Assets::Mesh::Meshlet::PKMeshlet* pMeshlets;
            uint32_t meshletCount;
            PK::Assets::Mesh::Meshlet::PKVertex* pVertices;
            uint32_t vertexCount;
            uint8_t* pIndices;
            uint32_t triangleCount;
        } 
        meshlet;
    };

    struct SubMeshStatic
    {
        Utilities::NameID name = 0u;
        uint32_t meshletFirst = 0u;
        uint32_t meshletCount = 0u;
        uint32_t vertexFirst = 0u;
        uint32_t vertexCount = 0u;
        uint32_t indexFirst = 0u;
        uint32_t indexCount = 0u;
        Math::BoundingBox bounds = Math::BoundingBox::GetMinBounds();
    };

    struct MeshStatic
    {
        MeshStaticCollection* baseMesh = nullptr;
        Utilities::NameID name = 0u;
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
        const SubMeshStatic* GetSubmesh(uint32_t localIndex) const;
        bool TryGetAccelerationStructureGeometryInfo(uint32_t localIndex, RHI::AccelerationStructureGeometryInfo* outInfo) const;
    };

    class MeshStaticCollection : public PK::Utilities::NoCopy
    {
        public:
            MeshStaticCollection();
            Buffer* GetPositionBuffer() const;
            Buffer* GetAttributeBuffer() const;
            Buffer* GetIndexBuffer() const;
            Buffer* GetMeshletVertexBuffer() const;
            Buffer* GetMeshletIndexBuffer() const;
            Buffer* GetMeshletSubmeshBuffer() const;
            Buffer* GetMeshletBuffer() const;
            const SubMeshStatic* GetSubmesh(uint32_t index) const;
            const bool HasPendingUpload() const;
            
            MeshStatic* Allocate(MeshStaticAllocationData* data);
            void Deallocate(MeshStatic* mesh);

            bool TryGetAccelerationStructureGeometryInfo(uint32_t globalSubmeshIndex, RHI::AccelerationStructureGeometryInfo* outInfo) const;

        private:
            Utilities::FixedPool<MeshStatic, 4096> m_staticMeshes;
            Utilities::FixedPool<SubMeshStatic, 8192> m_staticSubmeshes;

            uint32_t m_submeshCount = 0u;
            uint32_t m_meshletCount = 0u;
            uint32_t m_meshletVertexCount = 0u;
            uint32_t m_meshletTriangleCount = 0u;
            uint32_t m_vertexCount = 0u;
            uint32_t m_indexCount = 0u;

            RHI::ElementType m_indexType = RHI::ElementType::Uint;
            RHI::VertexStreamLayout m_streamLayout;

            BufferRef m_positionsBuffer;
            BufferRef m_attributesBuffer;
            BufferRef m_indexBuffer;

            BufferRef m_submeshBuffer;
            BufferRef m_meshletBuffer;
            BufferRef m_meshletVertexBuffer;
            BufferRef m_meshletIndexBuffer;
            mutable Utilities::FenceRef m_uploadFence;
    };
}
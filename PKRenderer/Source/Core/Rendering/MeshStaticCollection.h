#pragma once
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/FenceRef.h"
#include "Core/RHI/Layout.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
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
            BoundingBox bounds = BoundingBox::GetMinBounds();
        };

        NameID name = 0u;

        struct Regular
        {
            void* pVertices;
            void* pIndices;
            SubMesh* pSubmeshes;
            VertexStreamLayout streamLayout;
            ElementType indexType;
            uint32_t vertexCount;
            uint32_t indexCount;
            uint32_t submeshCount;
        } 
        regular;

        struct Meshlet
        {
            PKAssets::PKMeshletSubmesh* pSubmeshes;
            uint32_t submeshCount;
            PKAssets::PKMeshlet* pMeshlets;
            uint32_t meshletCount;
            PKAssets::PKMeshletVertex* pVertices;
            uint32_t vertexCount;
            uint8_t* pIndices;
            uint32_t triangleCount;
        } 
        meshlet;
    };

    struct SubMeshStatic
    {
        NameID name = 0u;
        uint32_t meshletFirst = 0u;
        uint32_t meshletCount = 0u;
        uint32_t vertexFirst = 0u;
        uint32_t vertexCount = 0u;
        uint32_t indexFirst = 0u;
        uint32_t indexCount = 0u;
        BoundingBox bounds = BoundingBox::GetMinBounds();
    };

    struct MeshStatic
    {
        MeshStaticCollection* baseMesh = nullptr;
        NameID name = 0u;
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
        bool TryGetAccelerationStructureGeometryInfo(uint32_t localIndex, AccelerationStructureGeometryInfo* outInfo) const;
    };

    class MeshStaticCollection : public NoCopy
    {
        public:
            MeshStaticCollection();
            RHIBuffer* GetPositionBuffer() const;
            RHIBuffer* GetAttributeBuffer() const;
            RHIBuffer* GetIndexBuffer() const;
            RHIBuffer* GetMeshletVertexBuffer() const;
            RHIBuffer* GetMeshletIndexBuffer() const;
            RHIBuffer* GetMeshletSubmeshBuffer() const;
            RHIBuffer* GetMeshletBuffer() const;
            const SubMeshStatic* GetSubmesh(uint32_t index) const;
            bool HasPendingUpload() const;
            
            MeshStatic* Allocate(MeshStaticAllocationData* data);
            void Deallocate(MeshStatic* mesh);

            bool TryGetAccelerationStructureGeometryInfo(uint32_t globalSubmeshIndex, AccelerationStructureGeometryInfo* outInfo) const;

        private:
            FixedPool<MeshStatic, 4096> m_staticMeshes;
            FixedPool<SubMeshStatic, 8192> m_staticSubmeshes;

            uint32_t m_submeshCount = 0u;
            uint32_t m_meshletCount = 0u;
            uint32_t m_meshletVertexCount = 0u;
            uint32_t m_meshletTriangleCount = 0u;
            uint32_t m_vertexCount = 0u;
            uint32_t m_indexCount = 0u;

            ElementType m_indexType = ElementType::Uint;
            VertexStreamLayout m_streamLayout;

            RHIBufferRef m_positionsBuffer;
            RHIBufferRef m_attributesBuffer;
            RHIBufferRef m_indexBuffer;

            RHIBufferRef m_submeshBuffer;
            RHIBufferRef m_meshletBuffer;
            RHIBufferRef m_meshletVertexBuffer;
            RHIBufferRef m_meshletIndexBuffer;
            mutable FenceRef m_uploadFence;
    };
}

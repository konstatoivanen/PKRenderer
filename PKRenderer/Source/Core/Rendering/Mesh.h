#pragma once
#include "Core/Utilities/FixedList.h"
#include "Core/Utilities/FenceRef.h"
#include "Core/Assets/Asset.h"
#include "Core/RHI/Layout.h"
#include "Core/Rendering/RenderingFwd.h"

namespace PK
{
    struct Mesh : public AssetWithImport<>
    {
        struct SubMesh
        {
            uint32_t vertexFirst = 0u;
            uint32_t vertexCount = 0u;
            uint32_t indexFirst = 0u;
            uint32_t indexCount = 0u;
            BoundingBox bounds = BoundingBox::GetMinBounds();
        };

        typedef FixedList<RHIBufferRef, PK_RHI_MAX_VERTEX_ATTRIBUTES> VertexBuffers;

        Mesh();
        Mesh(const RHIBufferRef& indexBuffer,
            ElementType indexType,
            RHIBufferRef* vertexBuffers,
            uint32_t vertexBufferCount,
            const VertexStreamLayout& streamLayout,
            SubMesh* submeshes,
            uint32_t submeshCount);

        void SetResources(const RHIBufferRef& indexBuffer,
            ElementType indexType,
            RHIBufferRef* vertexBuffers,
            uint32_t vertexBufferCount,
            const VertexStreamLayout& streamLayout,
            SubMesh* submeshes,
            uint32_t submeshCount);

        void AssetImport(const char* filepath) final;
        bool TryGetAccelerationStructureGeometryInfo(uint32_t submesh, AccelerationStructureGeometryInfo* outInfo);

        const VertexBuffers& GetVertexBuffers() const;
        const VertexStreamLayout& GetVertexStreamLayout() const;
        ElementType GetIndexType() const;
        const RHIBuffer* GetIndexBuffer() const;
        const SubMesh& GetSubmesh(int32_t submesh) const;
        uint32_t GetSubmeshCount() const;
        const SubMesh& GetFullRange() const;
        bool HasPendingUpload() const;

    private:
        VertexBuffers m_vertexBuffers;
        RHIBufferRef m_indexBuffer;
        mutable FenceRef m_uploadFence;

        VertexStreamLayout m_streamLayout;
        ElementType m_indexType;
        SubMesh m_fullRange{};
        uint32_t m_positionAttributeIndex = ~0u;
        std::vector<SubMesh> m_submeshes;
    };
}

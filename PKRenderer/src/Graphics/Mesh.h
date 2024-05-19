#pragma once
#include "Utilities/FixedList.h"
#include "Utilities/FenceRef.h"
#include "Math/Types.h"
#include "Core/Assets/Asset.h"
#include "Graphics/RHI/Layout.h"
#include "Graphics/GraphicsFwd.h"

namespace PK::Graphics
{
    struct Mesh : public Core::Assets::AssetWithImport<>
    {
        struct SubMesh
        {
            uint32_t vertexFirst = 0u;
            uint32_t vertexCount = 0u;
            uint32_t indexFirst = 0u;
            uint32_t indexCount = 0u;
            Math::BoundingBox bounds = Math::BoundingBox::GetMinBounds();
        };

        typedef Utilities::FixedList<BufferRef, RHI::PK_MAX_VERTEX_ATTRIBUTES> VertexBuffers;

        Mesh();
        Mesh(const BufferRef& indexBuffer,
            RHI::ElementType indexType,
            BufferRef* vertexBuffers,
            uint32_t vertexBufferCount,
            const RHI::VertexStreamLayout& streamLayout,
            SubMesh* submeshes,
            uint32_t submeshCount);

        void SetResources(const BufferRef& indexBuffer,
            RHI::ElementType indexType,
            BufferRef* vertexBuffers,
            uint32_t vertexBufferCount,
            const RHI::VertexStreamLayout& streamLayout,
            SubMesh* submeshes,
            uint32_t submeshCount);

        void AssetImport(const char* filepath) final;
        bool TryGetAccelerationStructureGeometryInfo(uint32_t submesh, RHI::AccelerationStructureGeometryInfo* outInfo);

        const VertexBuffers& GetVertexBuffers() const;
        const RHI::VertexStreamLayout& GetVertexStreamLayout() const;
        const RHI::ElementType GetIndexType() const;
        const Buffer* GetIndexBuffer() const;
        const SubMesh& GetSubmesh(int32_t submesh) const;
        const uint32_t GetSubmeshCount() const;
        const SubMesh& GetFullRange() const;
        const bool HasPendingUpload() const;

    private:
        VertexBuffers m_vertexBuffers;
        BufferRef m_indexBuffer;
        mutable Utilities::FenceRef m_uploadFence;

        RHI::VertexStreamLayout m_streamLayout;
        RHI::ElementType m_indexType;
        SubMesh m_fullRange{};
        uint32_t m_positionAttributeIndex = ~0u;
        std::vector<SubMesh> m_submeshes;
    };
}
#pragma once
#include "Core/Services/AssetDatabase.h"
#include "Rendering/RHI/GraphicsAPI.h"

namespace PK::Rendering::Objects
{
    // Note: Index ranges measured in triangles to allow for potentially more triangles than indices uint32_t would allow.
    // Or if meshlet data is packed further in the future.

    struct MeshletAllocationRange
    {
        uint32_t firstSubmesh;
        uint32_t firstMeshlet;
        uint32_t firstVertex;
        uint32_t firstTriangle;

        uint32_t submeshCount;
        uint32_t meshletCount;
        uint32_t vertexCount;
        uint32_t triangleCount;
    };

    class MeshletMesh : PK::Utilities::NoCopy
    {
        public:
            struct Range 
            { 
                uint32_t first; 
                uint32_t count; 

                constexpr bool operator < (const Range& other)
                {
                    return first < other.first;
                }
            };

            MeshletMesh(uint32_t maxSubmeshes, uint32_t maxMeshlets, uint32_t maxVertices, uint32_t maxTriangles);
            
            inline const RHI::Objects::Buffer* GetSubmeshBuffer() const { return m_submeshBuffer.get(); }
            inline const RHI::Objects::Buffer* GetMeshletBuffer() const { return m_meshletBuffer.get(); }
            inline const RHI::Objects::Buffer* GetVertexBuffer() const { return m_vertexBuffer.get(); }
            inline const RHI::Objects::Buffer* GetIndexBuffer() const { return m_indexBuffer.get(); }
            inline uint32_t GetSubmeshCount() const { return m_submeshCount; }
            inline uint32_t GetMeshletCount() const { return m_meshletCount; }
            inline uint32_t GetVertexCount() const { return m_vertexCount; }
            inline uint32_t GetTriangleCount() const { return m_triangleCount; }

            // Will write new offsets into provided pointers.
            MeshletAllocationRange Allocate
            (
                RHI::Objects::CommandBuffer* commandBuffer,
                Assets::Mesh::Meshlet::PKSubmesh* pSubmeshes,
                uint32_t submeshCount,
                Assets::Mesh::Meshlet::PKMeshlet* pMeshlets,
                uint32_t meshletCount,
                Assets::Mesh::Meshlet::PKVertex* pVertices,
                uint32_t vertexCount,
                uint8_t* pIndices,
                uint32_t triangleCount
            );

            void Deallocate(const MeshletAllocationRange& alloc);

        private:
            void SortAllocations();
            MeshletAllocationRange GetAllocationRange(uint32_t submeshCount, uint32_t meshletCount, uint32_t vertexCount, uint32_t triangleCount);

            RHI::Objects::BufferRef m_submeshBuffer;
            RHI::Objects::BufferRef m_meshletBuffer;
            RHI::Objects::BufferRef m_vertexBuffer;
            RHI::Objects::BufferRef m_indexBuffer;
            mutable RHI::FenceRef m_uploadFence;

            uint32_t m_submeshCount = 0u;
            uint32_t m_meshletCount = 0u;
            uint32_t m_vertexCount = 0u;
            uint32_t m_triangleCount = 0u;

            std::vector<Range> m_allocationsSubmesh;
            std::vector<Range> m_allocationsMeshlet;
            std::vector<Range> m_allocationsVertex;
            std::vector<Range> m_allocationsTriangle;
    };
}
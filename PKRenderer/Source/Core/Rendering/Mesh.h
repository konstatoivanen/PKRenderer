#pragma once
#include "Core/Utilities/FixedPool.h"
#include "Core/Utilities/FenceRef.h"
#include "Core/Utilities/List.h"
#include "Core/Rendering/RenderingFwd.h"
#include "Core/Assets/Asset.h"
#include "Core/RHI/Layout.h"

namespace PK
{
    typedef FixedList<RHIBufferRef, PK_RHI_MAX_VERTEX_ATTRIBUTES> VertexBuffers;

    struct SubMesh
    {
        NameID name = 0u;
        uint32_t vertexFirst = 0u;
        uint32_t vertexCount = 0u;
        uint32_t indexFirst = 0u;
        uint32_t indexCount = 0u;
        uint32_t meshletFirst = 0u;
        uint32_t meshletCount = 0u;
        AABB<float3> bounds = PK_FLOAT3_MIN_AABB;
    };
    
    struct MeshDescriptor
    {
        void* pVertices;
        void* pIndices;
        SubMesh* pSubmeshes;
        VertexStreamLayout streamLayout;
        uint32_t indexSize;
        uint32_t vertexCount;
        uint32_t indexCount;
        uint32_t submeshCount;
    };

    struct MeshletsDescriptor
    {
        PKAssets::PKMeshletSubmesh* pSubmeshes;
        uint32_t submeshCount;
        PKAssets::PKMeshlet* pMeshlets;
        uint32_t meshletCount;
        PKAssets::PKMeshletVertex* pVertices;
        uint32_t vertexCount;
        uint8_t* pIndices;
        uint32_t triangleCount;
    };

    struct MeshStaticDescriptor
    {
        NameID name = 0u;
        MeshDescriptor regular;
        MeshletsDescriptor meshlets;
    };


    struct IMesh
    {
        virtual ~IMesh() = 0;
        virtual const VertexBuffers& GetVertexBuffers() const = 0;
        virtual const RHIBuffer* GetIndexBuffer() const = 0;
        virtual const VertexStreamLayout& GetVertexStreamLayout() const = 0;
        virtual uint32_t GetIndexSize() const = 0;
        virtual uint32_t GetSubmeshCount() const = 0;
        virtual const SubMesh& GetSubmesh(int32_t submesh) const = 0;
    };

    struct IMeshlets
    {
        virtual ~IMeshlets() = 0;
        virtual RHIBuffer* GetMeshletVertexBuffer() const = 0;
        virtual RHIBuffer* GetMeshletIndexBuffer() const = 0;
        virtual RHIBuffer* GetMeshletSubmeshBuffer() const = 0;
        virtual RHIBuffer* GetMeshletBuffer() const = 0;
        virtual const SubMesh& GetSubmesh(int32_t index) const = 0;
    };

    struct IRayTracingGeometry
    {
        virtual ~IRayTracingGeometry() = 0;
        virtual bool HasPendingUpload() const = 0;
        virtual bool GatherRayTracingGeometry(uint32_t submesh, RayTracingGeometryInfo* outInfo) const = 0;
    };

    
    struct MeshStaticAllocator : public NoCopy
    {
        struct Allocation
        {
            MeshStaticAllocator* allocator = nullptr;
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
        };

        MeshStaticAllocator();

        constexpr RHIBuffer* GetMeshletVertexBuffer() const { return m_meshletVertexBuffer.get(); }
        constexpr RHIBuffer* GetMeshletIndexBuffer() const { return m_meshletIndexBuffer.get(); }
        constexpr RHIBuffer* GetMeshletSubmeshBuffer() const { return m_submeshBuffer.get(); }
        constexpr RHIBuffer* GetMeshletBuffer() const { return m_meshletBuffer.get(); }
        constexpr const VertexBuffers& GetVertexBuffers() const { return m_vertexBuffers; }
        constexpr RHIBuffer* GetIndexBuffer() const { return m_indexBuffer.get(); }
        constexpr const SubMesh& GetSubmesh(uint32_t index) const { return *m_submeshes[index]; }
        constexpr const VertexStreamLayout& GetVertexStreamLayout() const { return m_streamLayout; }
        constexpr uint32_t GetIndexSize() const { return m_indexSize; }
        inline bool HasPendingUpload() const { return !m_uploadFence.WaitInvalidate(0ull); }

        Allocation* Allocate(const MeshStaticDescriptor& desc);
        void Deallocate(Allocation* allocation);

        bool GatherRayTracingGeometry(uint32_t globalSubmeshIndex, RayTracingGeometryInfo* outInfo) const;

    private:
        VertexBuffers m_vertexBuffers;
        RHIBufferRef m_indexBuffer;
        RHIBufferRef m_submeshBuffer;
        RHIBufferRef m_meshletBuffer;
        RHIBufferRef m_meshletVertexBuffer;
        RHIBufferRef m_meshletIndexBuffer;
        FixedPool<Allocation, 4096ull> m_allocations;
        FixedPool<SubMesh, 8192ull> m_submeshes;
        VertexStreamLayout m_streamLayout;
        uint32_t m_indexSize = sizeof(uint32_t);
        uint32_t m_submeshCount = 0u;
        uint32_t m_meshletCount = 0u;
        uint32_t m_meshletVertexCount = 0u;
        uint32_t m_meshletTriangleCount = 0u;
        uint32_t m_vertexCount = 0u;
        uint32_t m_indexCount = 0u;
        int64_t m_preferredIndex = -1;
        mutable FenceRef m_uploadFence;
    };

    struct MeshStatic : public IMesh, public IMeshlets, public IRayTracingGeometry, public Asset
    {
        MeshStatic(MeshStaticAllocator* allocator, const char* filepath);
        MeshStatic(MeshStaticAllocator* allocator, const MeshStaticDescriptor& desc) : m_allocation(allocator->Allocate(desc)) {}
        MeshStatic(MeshStaticAllocator::Allocation* allocation) : m_allocation(allocation) {}
        MeshStatic(MeshStatic&& other);
        MeshStatic(MeshStatic const&) = delete;
        ~MeshStatic();

        constexpr MeshStaticAllocator* GetAllocator() const { return m_allocation->allocator; }
        inline uint32_t GetGlobalSubmeshIndex(uint32_t local) const { return m_allocation->submeshFirst + math::min(local, m_allocation->submeshCount - 1u); }
        inline RHIBuffer* GetMeshletVertexBuffer() const final { return m_allocation->allocator->GetMeshletVertexBuffer(); }
        inline RHIBuffer* GetMeshletIndexBuffer() const final { return m_allocation->allocator->GetMeshletIndexBuffer(); }
        inline RHIBuffer* GetMeshletSubmeshBuffer() const final { return m_allocation->allocator->GetMeshletSubmeshBuffer(); }
        inline RHIBuffer* GetMeshletBuffer() const final { return m_allocation->allocator->GetMeshletBuffer(); }
        inline const VertexBuffers& GetVertexBuffers() const final { return m_allocation->allocator->GetVertexBuffers(); }
        inline const RHIBuffer* GetIndexBuffer() const final { return m_allocation->allocator->GetIndexBuffer(); }
        inline const VertexStreamLayout& GetVertexStreamLayout() const final { return m_allocation->allocator->GetVertexStreamLayout(); }
        inline uint32_t GetIndexSize() const final { return m_allocation->allocator->GetIndexSize(); }
        inline uint32_t GetSubmeshCount() const final { return m_allocation->submeshCount; }
        inline const SubMesh& GetSubmesh(int32_t localIndex) const final { return m_allocation->allocator->GetSubmesh(GetGlobalSubmeshIndex((unsigned)localIndex)); }
        inline bool HasPendingUpload() const final { return m_allocation->allocator->HasPendingUpload(); }
        bool GatherRayTracingGeometry(uint32_t localIndex, RayTracingGeometryInfo* outInfo) const;
        
        private: 
            MeshStaticAllocator::Allocation* m_allocation = nullptr;
    };

    struct Mesh : public IMesh, public IRayTracingGeometry, public Asset
    {
        Mesh() {};
        Mesh(const char* filepath);
        Mesh(const MeshDescriptor& descriptor, const char* name);
        Mesh(const RHIBufferRef& indexBuffer,
            uint32_t indexSize,
            const VertexStreamLayout& streamLayout,
            RHIBufferRef* vertexBuffers,
            uint32_t vertexBufferCount,
            SubMesh* submeshes,
            uint32_t submeshCount);

        void SetResources(const MeshDescriptor& descriptor, const char* name);

        void SetResources(const RHIBufferRef& indexBuffer, 
            uint32_t indexSize, 
            const VertexStreamLayout& streamLayout,
            RHIBufferRef* vertexBuffers, 
            uint32_t vertexBufferCount,
            SubMesh* submeshes,
            uint32_t submeshCount);

        inline const VertexBuffers& GetVertexBuffers() const final { return m_vertexBuffers; }
        inline const RHIBuffer* GetIndexBuffer() const final { return m_indexBuffer.get(); }
        inline const VertexStreamLayout& GetVertexStreamLayout() const final { return m_streamLayout; }
        inline uint32_t GetIndexSize() const final { return m_indexSize; }
        inline uint32_t GetSubmeshCount() const final { return math::max(1u, (uint32_t)m_submeshes.GetCount()); }
        const SubMesh& GetSubmesh(int32_t submesh) const final;
        inline bool HasPendingUpload() const final { return !m_uploadFence.WaitInvalidate(0ull); }
        bool GatherRayTracingGeometry(uint32_t submesh, RayTracingGeometryInfo* outInfo) const final;

    private:
        VertexBuffers m_vertexBuffers;
        RHIBufferRef m_indexBuffer;
        VertexStreamLayout m_streamLayout;
        uint32_t m_indexSize;
        List<SubMesh, 1ull> m_submeshes;
        SubMesh m_fullrange{};
        uint32_t m_positionAttributeIndex = ~0u;
        mutable FenceRef m_uploadFence;
    };
}

#pragma once
#include <PKAssets/PKAsset.h>
#include "Core/Rendering/RenderingFwd.h"
#include "Core/Math/Math.h"

namespace PK { class AssetDatabase; }

namespace PK::MeshUtilities
{
    struct MeshletBuildData
    {
        PKAssets::PKMeshletSubmesh submesh;
        uint32_t meshlet_count = 0u;
        uint32_t vertex_count = 0u;
        uint32_t index_count = 0u;
        
        void* buffer = nullptr;
        uint8_t* indices;
        PKAssets::PKMeshletVertex* vertices;
        PKAssets::PKMeshlet* meshlets;

        MeshletBuildData(size_t count_meshlet);
        MeshletBuildData(MeshletBuildData&& other);
        MeshletBuildData(MeshletBuildData const&) = delete;
        ~MeshletBuildData();
        MeshletBuildData& operator=(MeshletBuildData const&) = delete;
    };

    // Not representative of GPU buffer layouts.
    // Ordered this way to satisfy vector alignment requirements.
    struct alignas(16) VertexDefault
    {
        float3 in_POSITION;
        float3 in_NORMAL;
        float2 in_TEXCOORD0;
        float4 in_TANGENT;
    };

    struct GeometryContext
    {
        VertexDefault* pVertices = nullptr;
        uint32_t* pIndices = nullptr;
        uint32_t countVertex = 0u;
        uint32_t countIndex = 0u;
        AABB<float3> aabb;
    };

    template<typename Ts, typename Td>
    void ConvertVertices(void* dst, const void* src, size_t count, size_t strideSrc, size_t strideDst)
    {
        for (auto i = 0ull; i < count; ++i)
        {
            const void* vsrc = reinterpret_cast<const char*>(src) + strideSrc * i;
            void* vdst = reinterpret_cast<char*>(dst) + strideDst * i;

            // Filty UB here. should use memcpy instead.
            if constexpr (TIsSame<Ts, float> && TIsSame<Td, uint16_t>)
            {
                *reinterpret_cast<uint16_t*>(vdst) = math::f32tof16(*reinterpret_cast<const float*>(vsrc));
            }
            else if constexpr (TIsSame<Ts, uint16_t> && TIsSame<Td, float>)
            {
                *reinterpret_cast<float*>(vdst) = math::f16tof32(*reinterpret_cast<const uint16_t*>(vsrc));
            }
            else if constexpr (TIsSame<Ts, float> && TIsSame<Td, uint32_t>)
            {
                *reinterpret_cast<uint32_t*>(vdst) = math::asuint(*reinterpret_cast<const float*>(vsrc));
            }
            else if constexpr (TIsSame<Ts, uint32_t> && TIsSame<Td, float>)
            {
                *reinterpret_cast<float*>(vdst) = math::asfloat(*reinterpret_cast<const uint32_t*>(vsrc));
            }
            else
            {
                *reinterpret_cast<Td*>(vdst) = static_cast<Td>(*reinterpret_cast<const Ts*>(vsrc));
            }
        }
    }

    void AlignVertexStreams(char* vertices, size_t count, const VertexStreamLayout& src, const VertexStreamLayout& dst);

    void CalculateNormals(GeometryContext* ctx, float sign = 1.0f);

    void CalculateTangents(GeometryContext* ctx);

    MeshletBuildData BuildMeshletsMonotone(GeometryContext* ctx);
    MeshStatic CreateMeshStatic(MeshStaticAllocator* allocator, GeometryContext* ctx, const char* name);
    MeshStatic CreateBoxMeshStatic(MeshStaticAllocator* allocator, const float3& offset, const float3& extents);
    MeshStatic CreateQuadMeshStatic(MeshStaticAllocator* allocator, const float2& min, const float2& max);
    MeshStatic CreatePlaneMeshStatic(MeshStaticAllocator* allocator, const float2& center, const float2& extents, uint2 resolution);
    MeshStatic CreateSphereMeshStatic(MeshStaticAllocator* allocator, const float3& offset, const float radius);
}

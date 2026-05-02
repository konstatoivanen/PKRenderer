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

    struct GeometryContext
    {
        void* pVertices = nullptr;
        float* pPositions = nullptr;
        uint32_t stridePositionsf32 = 0u;
        float* pNormals = nullptr;
        uint32_t strideNormalsf32 = 0u;
        float* pTangents = nullptr;
        uint32_t strideTangentsf32 = 0u;
        float* pTexcoords = nullptr;
        uint32_t strideTexcoordsf32 = 0u;
        uint32_t* pIndices = nullptr;
        uint32_t countVertex = 0u;
        uint32_t countIndex = 0u;
        AABB<float3> aabb;
    };

    struct Vertex_Full
    {
        float3 in_POSITION;
        float3 in_NORMAL;
        float4 in_TANGENT;
        float2 in_TEXCOORD0;
    };

    struct Vertex_Lite
    {
        float3 in_POSITION;
        float2 in_TEXCOORD0;
    };

    struct Vertex_Position
    {
        float3 in_POSITION;
    };

    struct Vertex_NormalTangentTexCoord
    {
        float3 in_NORMAL;
        float4 in_TANGENT;
        float2 in_TEXCOORD0;
    };

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

#pragma once
#include <vector>
#include <PKAssets/PKAsset.h>
#include "Core/Utilities/ForwardDeclare.h"
#include "Core/Math/Math.h"
#include "Core/Rendering/RenderingFwd.h"

PK_FORWARD_DECLARE_IN_NAMESPACE(PK, class AssetDatabase);

namespace PK::MeshUtilities
{
    struct MeshletBuildData
    {
        PKAssets::PKMeshletSubmesh submesh;
        std::vector<PKAssets::PKMeshlet> meshlets;
        std::vector<PKAssets::PKMeshletVertex> vertices;
        std::vector<uint8_t> indices;
        uint32_t meshlet_count = 0u;
        uint32_t vertex_count = 0u;
        uint32_t index_count = 0u;
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
        BoundingBox aabb;
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
    MeshStatic* CreateMeshStatic(MeshStaticCollection* baseMesh, GeometryContext* ctx, const char* name);
    MeshStatic* CreateBoxMeshStatic(MeshStaticCollection* baseMesh, const float3& offset, const float3& extents);
    MeshStatic* CreateQuadMeshStatic(MeshStaticCollection* baseMesh, const float2& min, const float2& max);
    MeshStatic* CreatePlaneMeshStatic(MeshStaticCollection* baseMesh, const float2& center, const float2& extents, uint2 resolution);
    MeshStatic* CreateSphereMeshStatic(MeshStaticCollection* baseMesh, const float3& offset, const float radius);
}

#pragma once
#include <vector>
#include <PKAssets/PKAsset.h>
#include "Utilities/ForwardDeclare.h"
#include "Math/Types.h"
#include "Graphics/GraphicsFwd.h"

namespace PK::Graphics
{
    struct MeshletBuildData
    {
        PK::Assets::Mesh::Meshlet::PKSubmesh submesh;
        std::vector<PK::Assets::Mesh::Meshlet::PKMeshlet> meshlets;
        std::vector<PK::Assets::Mesh::Meshlet::PKVertex> vertices;
        std::vector<uint8_t> indices;
    };

    struct GeometryContext
    {
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
        Math::BoundingBox aabb;
    };

    struct Vertex_Full
    {
        Math::float3 in_POSITION;
        Math::float3 in_NORMAL;
        Math::float4 in_TANGENT;
        Math::float2 in_TEXCOORD0;
    };

    struct Vertex_Lite
    {
        Math::float3 in_POSITION;
        Math::float2 in_TEXCOORD0;
    };

    struct Vertex_Position
    {
        Math::float3 in_POSITION;
    };

    struct Vertex_NormalTangentTexCoord
    {
        Math::float3 in_NORMAL;
        Math::float4 in_TANGENT;
        Math::float2 in_TEXCOORD0;
    };

    void AlignVertexStreams(char* vertices, size_t count, const RHI::VertexStreamLayout& src, const RHI::VertexStreamLayout& dst);

    void CalculateNormals(GeometryContext* ctx, float sign = 1.0f);

    void CalculateTangents(GeometryContext* ctx);

    MeshletBuildData BuildMeshletsMonotone(GeometryContext* ctx);

    MeshStaticAssetRef CreateMeshStaticAsset(MeshStaticCollection* baseMesh, GeometryContext* ctx, const char* name);
    MeshStaticAssetRef CreateBoxMeshStaticAsset(MeshStaticCollection* baseMesh, const Math::float3& offset, const Math::float3& extents);
    MeshStaticAssetRef CreateQuadMeshStaticAsset(MeshStaticCollection* baseMesh, const Math::float2& min, const Math::float2& max);
    MeshStaticAssetRef CreatePlaneMeshStaticAsset(MeshStaticCollection* baseMesh, const Math::float2& center, const Math::float2& extents, Math::uint2 resolution);
    MeshStaticAssetRef CreateSphereMeshStaticAsset(MeshStaticCollection* baseMesh, const Math::float3& offset, const float radius);
}

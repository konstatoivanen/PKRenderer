#pragma once
#include <PKAssets/PKAsset.h>
#include "Math/Types.h"

namespace PK::Rendering::Geometry
{
    struct MeshletBuildData
    {
        PK::Assets::Mesh::Meshlet::PKSubmesh submesh;
        std::vector<Assets::Mesh::Meshlet::PKMeshlet> meshlets;
        std::vector<Assets::Mesh::Meshlet::PKVertex> vertices;
        std::vector<uint8_t> indices;
    };

    MeshletBuildData BuildMeshletsMonotone(const float* pPositions,
        const float* pTexcoords,
        const float* pNormals,
        const float* pTangents,
        const uint32_t* indices,
        uint32_t vertexStride,
        uint32_t vertexCount,
        uint32_t indexCount,
        const Math::BoundingBox& aabb);
}
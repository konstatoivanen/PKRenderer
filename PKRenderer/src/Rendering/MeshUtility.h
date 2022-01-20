#pragma once
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/VirtualMesh.h"
#include "Math/Types.h"

namespace PK::Rendering::MeshUtility
{
    using namespace Utilities;
    using namespace PK::Math;
    using namespace PK::Rendering::Objects;

    void CalculateNormals(const float3* vertices, const uint* indices, float3* normals, uint vcount, uint icount, float sign = 1.0f);
    void CalculateTangents(const float3* vertices, const float3* normals, const float2* texcoords, const uint* indices, float4* tangents, uint vcount, uint icount);
    void CalculateTangents(void* vertices, uint stride, uint vertexOffset, uint normalOffset, uint tangentOffset, uint texcoordOffset, const uint* indices, uint vcount, uint icount);
    Ref<Mesh> GetBox(const float3& offset, const float3& extents);
    Ref<Mesh> GetQuad(const float2& min, const float2& max);
    Ref<VirtualMesh> GetPlane(Ref<Mesh> baseMesh, const float2& center, const float2& extents, uint2 resolution);
    Ref<VirtualMesh> GetSphere(Ref<Mesh> baseMesh, const float3& offset, const float radius);
}

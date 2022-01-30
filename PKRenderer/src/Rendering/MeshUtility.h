#pragma once
#include "Rendering/Objects/Mesh.h"
#include "Rendering/Objects/VirtualMesh.h"
#include "Math/Types.h"

namespace PK::Rendering::MeshUtility
{
    void CalculateNormals(const Math::float3* vertices, const uint32_t* indices, Math::float3* normals, uint32_t vcount, uint32_t icount, float sign = 1.0f);
    void CalculateTangents(const Math::float3* vertices, const Math::float3* normals, const Math::float2* texcoords, const uint32_t indices, Math::float4* tangents, uint32_t vcount, uint32_t icount);
    void CalculateTangents(void* vertices, uint32_t stride, uint32_t vertexOffset, uint32_t normalOffset, uint32_t tangentOffset, uint32_t texcoordOffset, const uint32_t* indices, uint32_t vcount, uint32_t icount);
    Utilities::Ref<Objects::Mesh> GetBox(const Math::float3& offset, const Math::float3& extents);
    Utilities::Ref<Objects::Mesh> GetQuad(const Math::float2& min, const Math::float2& max);
    Utilities::Ref<Objects::VirtualMesh> GetPlane(Utilities::Ref<Objects::Mesh> baseMesh, const Math::float2& center, const Math::float2& extents, Math::uint2 resolution);
    Utilities::Ref<Objects::VirtualMesh> GetSphere(Utilities::Ref<Objects::Mesh> baseMesh, const Math::float3& offset, const float radius);
}

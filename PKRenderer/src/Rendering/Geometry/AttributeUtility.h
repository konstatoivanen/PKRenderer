#pragma once
#include "Math/Types.h"

namespace PK::Rendering::Geometry
{
    void CalculateNormals(const Math::float3* vertices, 
                          const uint32_t* indices, 
                          Math::float3* normals, 
                          uint32_t vcount, 
                          uint32_t icount, 
                          float sign = 1.0f);

    void CalculateTangents(const Math::float3* vertices, 
                           const Math::float3* normals, 
                           const Math::float2* texcoords, 
                           const uint32_t* indices, 
                           Math::float4* tangents, 
                           uint32_t vcount, 
                           uint32_t icount);

    void CalculateTangents(void* vertices, 
                           uint32_t stride, 
                           uint32_t vertexOffset, 
                           uint32_t normalOffset, 
                           uint32_t tangentOffset, 
                           uint32_t texcoordOffset, 
                           const uint32_t* indices, 
                           uint32_t vcount, 
                           uint32_t icount);
}

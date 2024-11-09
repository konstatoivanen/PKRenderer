#pragma once

#include "Utilities.glsl"

#define GIZMOS_MAX_VERTICES 16384u

PK_DECLARE_BUFFER(uint4, pk_Gizmos_IndirectVertices, PK_SET_GLOBAL);
PK_DECLARE_VARIABLE(uint4, pk_Gizmos_IndirectArguments, PK_SET_GLOBAL);

void Gizmos_DrawPacked(uint4 v0, uint4 v1)
{
    uint vertexIndex = atomicAdd(PK_VARIABLE_DATA(pk_Gizmos_IndirectArguments).x, 2u);
    vertexIndex %= GIZMOS_MAX_VERTICES;
    PK_BUFFER_DATA(pk_Gizmos_IndirectVertices, vertexIndex + 0u) = v0;
    PK_BUFFER_DATA(pk_Gizmos_IndirectVertices, vertexIndex + 1u) = v1;
}

void Gizmos_DrawLine(float3 start, float3 end, float4 color)
{
    Gizmos_DrawPacked(uint4(floatBitsToUint(start), packUnorm4x8(color)), uint4(floatBitsToUint(end), packUnorm4x8(color)));
}

void Gizmos_DrawRay(float3 origin, float3 vector, float4 color)
{
    Gizmos_DrawLine(origin, origin + vector, color);
}

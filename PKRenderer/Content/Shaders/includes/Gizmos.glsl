#pragma once

#include "Utilities.glsl"
#include "Common.glsl"

#define GIZMOS_MAX_VERTICES 16384u

buffer<uint4> pk_Gizmos_IndirectVertices;
buffer<uint4,1u> pk_Gizmos_IndirectArguments;

void Gizmos_DrawPacked(uint4 v0, uint4 v1)
{
    uint vertexIndex = atomicAdd(pk_Gizmos_IndirectArguments.x, 2u);
    vertexIndex %= GIZMOS_MAX_VERTICES;
    pk_Gizmos_IndirectVertices[vertexIndex + 0u] = v0;
    pk_Gizmos_IndirectVertices[vertexIndex + 1u] = v1;
}

void Gizmos_DrawWorldLine(float3 start, float3 end, float4 color)
{
    Gizmos_DrawPacked(uint4(floatBitsToUint(start), packUnorm4x8(color)), uint4(floatBitsToUint(end), packUnorm4x8(color)));
}

void Gizmos_DrawWorldRay(float3 origin, float3 vector, float4 color)
{
    Gizmos_DrawWorldLine(origin, origin + vector, color);
}

void Gizmos_DrawClipUvwLine(float3 start, float3 end, float4 color)
{
    const float3 p0 = UvToWorldPos(start.xy, start.z);
    const float3 p1 = UvToWorldPos(end.xy, end.z);
    Gizmos_DrawWorldLine(p0, p1, color);
}

void Gizmos_DrawClipUvwRay(float3 origin, float3 vector, float4 color)
{
    Gizmos_DrawClipUvwLine(origin, origin + vector, color);
}

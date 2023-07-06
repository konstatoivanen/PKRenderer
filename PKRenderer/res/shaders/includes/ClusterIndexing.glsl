#pragma once
#include Common.glsl

#define LIGHT_CLUSTER_TILE_COUNT_X 16
#define LIGHT_CLUSTER_TILE_COUNT_Y 9
#define LIGHT_CLUSTER_TILE_COUNT_Z 24
#define LIGHT_CLUSTER_TILE_COUNT_XY float2(16.0f, 9.0f)
#define LIGHT_CLUSTER_GROUP_SIZE_Z 4
#define LIGHT_CLUSTER_GROUP_SIZE_XYZ 576 // 16 * 9 * 4
#define LIGHT_CLUSTER_TILE_MAX_LIGHT_COUNT 128

float ZCoordToLinearDepth(float index)
{
    return pk_ProjectionParams.x * pow(pk_ExpProjectionParams.z, index / LIGHT_CLUSTER_TILE_COUNT_Z);
}

int3 GetTileIndexUV(float2 uv, float lineardepth)
{
    // Source: http://www.aortiz.me/2018/12/21/CG.html
    int zTile = int(log2(lineardepth) * (LIGHT_CLUSTER_TILE_COUNT_Z * pk_ExpProjectionParams.x) + (LIGHT_CLUSTER_TILE_COUNT_Z * pk_ExpProjectionParams.y));
    return int3(int2(uv * LIGHT_CLUSTER_TILE_COUNT_XY), max(zTile, 0));
}

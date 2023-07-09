#pragma once
#include Common.glsl

#define LIGHT_CLUSTER_TILE_COUNT_X 16
#define LIGHT_CLUSTER_TILE_COUNT_Y 9
#define LIGHT_CLUSTER_TILE_COUNT_Z 24
#define LIGHT_CLUSTER_TILE_COUNT_XY float2(16.0f, 9.0f)
#define LIGHT_CLUSTER_GROUP_SIZE_Z 4
#define LIGHT_CLUSTER_GROUP_SIZE_XYZ 576 // 16 * 9 * 4
#define LIGHT_CLUSTER_TILE_MAX_LIGHT_COUNT 128

int3 GetTileIndexUV(float2 uv, float viewdepth)
{
    const int z = max(0, int(LIGHT_CLUSTER_TILE_COUNT_Z * ClipDepthExp(viewdepth)));
    return int3(uv * LIGHT_CLUSTER_TILE_COUNT_XY, z);
}

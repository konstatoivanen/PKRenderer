#version 460
#pragma PROGRAM_COMPUTE
#include includes/SharedVolumeFog.glsl

const int2 offsets[4] = { int2(0,0), int2(0,1), int2(1,1), int2(1,0) };

layout(local_size_x = VOLUME_DEPTH_BATCH_SIZE_PX, local_size_y = VOLUME_DEPTH_BATCH_SIZE_PX, local_size_z = 1) in;
void main()
{
    int2 coord = int2(gl_GlobalInvocationID.xy) * 2;
    coord.x = min(coord.x, int(pk_ScreenParams.x - 2));
    coord.y = min(coord.y, int(pk_ScreenParams.y - 2));
    float2 uv = coord * pk_ScreenParams.zw;

    float4 depths = LinearizeDepth(textureGatherOffsets(pk_ScreenDepth, uv, offsets));

    float depth = depths.x;
    depth = max(depth, depths.y);
    depth = max(depth, depths.z);
    depth = max(depth, depths.w);

    // Gathering can exceed the tile bounds by 1px. A negligible margin of error.
    atomicMax(PK_BUFFER_DATA(pk_VolumeMaxDepths, GetVolumeDepthTileIndex(uv)), floatBitsToUint(depth));
}
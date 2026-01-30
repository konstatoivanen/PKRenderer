#pragma pk_program SHADER_STAGE_COMPUTE ClearCs
#include "includes/Common.glsl"
#include "includes/SceneGIVX.glsl"

PK_DECLARE_SET_DRAW uniform image3D pk_Image;

[numthreads(4u, 4u, 4u)]
void ClearCs()
{
    const float3 world_position = GI_VoxelToWorldSpace(int3(gl_GlobalInvocationID));
    const float3 clip_uvw = WorldToClipUvw(world_position);

    if (!Test_InUvw(clip_uvw))
    {
        return;
    }

    uint write_count = imageLoad(pk_GI_VolumeMaskWrite, int3(gl_GlobalInvocationID)).x;
    imageStore(pk_GI_VolumeMaskWrite, int3(gl_GlobalInvocationID), uint4(0u));

    if (write_count == 0u)
    {
        imageStore(pk_Image, int3(gl_GlobalInvocationID), 0.0f.xxxx);
    }
}
#pragma pk_program SHADER_STAGE_COMPUTE main
#include "includes/Common.glsl"
#include "includes/SceneGIVX.glsl"

PK_DECLARE_SET_DRAW uniform image3D pk_Image;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
void main()
{
    const float3 world_position = GI_VoxelToWorldSpace(int3(gl_GlobalInvocationID));
    const float3 clip_uvw = WorldToClipUVW(world_position);

    if (!Test_InUVW(clip_uvw))
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
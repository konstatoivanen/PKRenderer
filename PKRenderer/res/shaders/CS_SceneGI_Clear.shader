#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl

layout(rgba16, set = PK_SET_DRAW) uniform image3D _DestinationTex;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
void main()
{
    float3 worldpos = VoxelToWorldSpace(int3(gl_GlobalInvocationID));

    if (!Test_WorldToClipSpace(worldpos))
    {
        return;
    }

    uint writeCount = imageLoad(pk_SceneGI_VolumeMaskWrite, int3(gl_GlobalInvocationID)).x;
    imageStore(pk_SceneGI_VolumeMaskWrite, int3(gl_GlobalInvocationID), uint4(0u));

    if (writeCount == 0u)
    {
        imageStore(_DestinationTex, int3(gl_GlobalInvocationID), 0.0f.xxxx);
    }
}
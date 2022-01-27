#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl

layout(rgba16, set = PK_SET_DRAW) uniform image3D _DestinationTex;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;
void main()
{
    float3 worldpos = VoxelToWorldSpace(int3(gl_GlobalInvocationID));

    if (!WorldToClipSpaceCull(worldpos, 0.05f))
    {
        return;
    }

    float4 value = imageLoad(_DestinationTex, int3(gl_GlobalInvocationID));
    value.rgb *= value.a * pk_SceneGI_Fade;
    value.rgb = floor(value.rgb * 0xFFFF.xxx) / 0xFFFF.xxx;
    imageStore(_DestinationTex, int3(gl_GlobalInvocationID), value);
}
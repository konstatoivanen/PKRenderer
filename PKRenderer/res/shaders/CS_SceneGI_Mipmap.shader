#version 460
#pragma PROGRAM_COMPUTE
#include includes/Utilities.glsl

PK_DECLARE_SET_DRAW uniform sampler3D _SourceTex;
layout(rgba16, set = PK_SET_DRAW) uniform writeonly restrict image3D _DestinationTex;

layout(local_size_x = 4, local_size_y = 4, local_size_z = 4) in;
void main()
{
    uint3 baseSize = uint3(textureSize(_SourceTex, 0).xyz);
    uint3 levelSize = gl_NumWorkGroups.xyz * gl_WorkGroupSize.xyz;
    int level = int(log2(float(baseSize.x)) - log2(float(levelSize.x))) - 1;
    float3 uvw = (float3(gl_GlobalInvocationID) + 0.5f.xxx) / float3(levelSize);
    imageStore(_DestinationTex, int3(gl_GlobalInvocationID), tex2DLod(_SourceTex, uvw, level));
}
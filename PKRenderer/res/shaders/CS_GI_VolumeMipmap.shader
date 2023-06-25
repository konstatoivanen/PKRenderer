#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl

PK_DECLARE_SET_DRAW uniform sampler3D _SourceTex;
layout(rgba16f, set = PK_SET_DRAW) uniform writeonly restrict image3D _DestinationTex;
layout(rgba16f, set = PK_SET_SHADER) uniform writeonly restrict image3D _DestinationMip1;
layout(rgba16f, set = PK_SET_SHADER) uniform writeonly restrict image3D _DestinationMip2;
layout(rgba16f, set = PK_SET_SHADER) uniform writeonly restrict image3D _DestinationMip3;

#define GROUP_SIZE 4u
shared float4 lds_Data[GROUP_SIZE * GROUP_SIZE * GROUP_SIZE];

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE, local_size_z = GROUP_SIZE) in;
void main()
{
    const uint thread = gl_LocalInvocationIndex;
    const uint3 localCoord = gl_LocalInvocationID;
    const uint3 baseSize = uint3(textureSize(_SourceTex, 0).xyz);
    const uint3 levelSize = gl_NumWorkGroups.xyz * gl_WorkGroupSize.xyz;
    const int level = int(log2(float(baseSize.x)) - log2(float(levelSize.x))) - 1;
    const float3 uvw = (gl_GlobalInvocationID + 1.0f.xxx) / levelSize;

    float4 local = lds_Data[thread] = tex2DLod(_SourceTex, uvw, level);
    imageStore(_DestinationTex, int3(gl_GlobalInvocationID), local);
    barrier();

    // Cant use binary mask due to 3d coordinates
    if (All_Equal(gl_LocalInvocationID % 2, uint3(0u)))
    {
        local += lds_Data[thread + 0x1u];
        local += lds_Data[thread + 0x4u];
        local += lds_Data[thread + 0x5u];
        local += lds_Data[thread + 0x10u];
        local += lds_Data[thread + 0x11u];
        local += lds_Data[thread + 0x14u];
        local += lds_Data[thread + 0x15u];
        local /= 8.0f;

        lds_Data[thread] = local;
        imageStore(_DestinationMip1, int3(gl_GlobalInvocationID) >> 1, local);
    }
    barrier();

    if (thread == 0u)
    {
        local += lds_Data[thread + 0x2u];
        local += lds_Data[thread + 0x8u];
        local += lds_Data[thread + 0xau];
        local += lds_Data[thread + 0x20u];
        local += lds_Data[thread + 0x22u];
        local += lds_Data[thread + 0x28u];
        local += lds_Data[thread + 0x2au];
        local /= 8.0f;
        imageStore(_DestinationMip2, int3(gl_GlobalInvocationID) >> 2, local);
    }
}
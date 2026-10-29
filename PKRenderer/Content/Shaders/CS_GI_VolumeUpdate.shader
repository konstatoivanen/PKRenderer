
#pragma pk_multi_compile PASS_MIP PASS_CLEAR
#pragma pk_program Compute VolumeMipmapCs PASS_MIP
#pragma pk_program Compute VolumeClearCs PASS_CLEAR

#include "includes/Common.glsl"
#include "includes/SceneGIVX.glsl"

uniform Texture3DRef pk_Texture;
uniform RWTexture3DRef<float4> pk_Image;
uniform RWTexture3DRef<float4> pk_Image1;
uniform RWTexture3DRef<float4> pk_Image2;

groupshared float4 lds_Data[64u];

[numthreads(4u, 4u, 4u)]
void VolumeMipmapCs()
{
    const uint thread = gl_LocalInvocationIndex;
    const uint3 size_base = pk_Texture.GetSize(0);
    const uint3 size_level = gl_NumWorkGroups.xyz * gl_WorkGroupSize.xyz;
    const int level = int(log2(float(size_base.x)) - log2(float(size_level.x))) - 1;
    const float3 uvw = (gl_GlobalInvocationID + 0.5f.xxx) / size_level;

    float4 local = lds_Data[thread] = pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uvw, level);
    pk_Image[int3(gl_GlobalInvocationID)] = local;

    GroupMemoryBarrierWithGroupSync();

    // Cant use binary mask due to 3d coordinates
    if (all((gl_LocalInvocationID % 2u) == 0u))
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
        pk_Image1[int3(gl_GlobalInvocationID) >> 1] = local;
    }

    GroupMemoryBarrierWithGroupSync();

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
        pk_Image2[int3(gl_GlobalInvocationID) >> 2] = local;
    }
}

[numthreads(4u, 4u, 4u)]
void VolumeClearCs()
{
    const float3 world_position = GI_VX_CoordToWorld(int3(gl_GlobalInvocationID));
    const float3 clip_uvw = WorldToClipUvw(world_position);

    if (!Test_InUvw(clip_uvw))
    {
        return;
    }

    uint write_count = pk_GI_VX_Mask[int3(gl_GlobalInvocationID)];
    pk_GI_VX_Mask[int3(gl_GlobalInvocationID)] = 0u;

    if (write_count == 0u)
    {
        pk_Image[int3(gl_GlobalInvocationID)] = 0.0f.xxxx;
    }
}

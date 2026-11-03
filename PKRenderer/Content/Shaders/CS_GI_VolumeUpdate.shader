
#pragma pk_multi_compile PASS_MIP PASS_CLEAR
#pragma pk_program Compute VolumeMipmapCs PASS_MIP
#pragma pk_program Compute VolumeClearCs PASS_CLEAR

#include "includes/Common.glsl"
#include "includes/SceneGIVX.glsl"

uniform Texture3D<float4> pk_Texture;
uniform RWTexture3D<float4> pk_Image;
uniform RWTexture3D<float4> pk_Image1;
uniform RWTexture3D<float4> pk_Image2;

groupshared float4 lds_Data[64u];

[numthreads(4u, 4u, 4u)]
void VolumeMipmapCs(uint thread : SV_GroupIndex, int3 coord : SV_DispatchThreadID, uint3 local_coord : SV_GroupThreadID)
{
    const uint3 size_base = pk_Texture.GetSize();
    const uint3 size_level = gl_NumWorkGroups.xyz * gl_WorkGroupSize.xyz;
    const int level = int(log2(float(size_base.x)) - log2(float(size_level.x))) - 1;
    const float3 uvw = (coord + 0.5f.xxx) / size_level;

    float4 local = lds_Data[thread] = pk_Texture.SampleLevel(pk_SamplerBilinearClamped, uvw, level);
    pk_Image[coord] = local;

    GroupMemoryBarrierWithGroupSync();

    // Cant use binary mask due to 3d coordinates
    if (all((local_coord % 2u) == uint3(0u)))
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
        pk_Image1[coord >> 1] = local;
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
        pk_Image2[coord >> 2] = local;
    }
}

[numthreads(4u, 4u, 4u)]
void VolumeClearCs(int3 coord : SV_DispatchThreadID)
{
    const float3 world_position = GI_VX_CoordToWorld(coord);
    const float3 clip_uvw = WorldToClipUvw(world_position);

    if (!Test_InUvw(clip_uvw))
    {
        return;
    }

    uint write_count = pk_GI_VX_Mask[coord].x;
    pk_GI_VX_Mask[coord] = 0u;

    if (write_count == 0u)
    {
        pk_Image[coord] = 0.0f.xxxx;
    }
}

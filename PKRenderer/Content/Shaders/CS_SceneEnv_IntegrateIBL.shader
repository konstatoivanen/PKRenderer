
#pragma pk_program Compute IntegrateCs

#include "includes/Common.glsl"
#include "includes/ComputeQuadSwap.glsl"
#include "includes/SceneEnv.glsl"
#include "includes/VolumetricFog.glsl"
#include "includes/Encoding.glsl"

#define PK_STATIC_FOG_VIRTUAL_DISTANCE 14000.0f

uniform RWTexture2DRef<uint> pk_Image;
uniform RWTexture2DRef<uint> pk_Image1;
uniform RWTexture2DRef<uint> pk_Image2;
uniform RWTexture2DRef<uint> pk_Image3;
uniform RWTexture2DRef<uint> pk_Image4;
uniform float pk_SceneEnv_Exposure;
uniform float4 pk_SceneEnv_Origin;

groupshared float3 lds_irrad;
groupshared float3 lds_trans;

[numthreads(8u, 8u, 1u)]
void IntegrateCs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const uint thread = gl_LocalInvocationIndex;
    const float2 texel_size = 1.0f.xx / float2(pk_Image.GetSize());
    const float2 uv_base = (coord + 1.0f.xx) * texel_size;

    float3 local_irrad = 0.0f.xxx;
    float3 local_trans = 0.0f.xxx;

    for (uint yy = 0u; yy < 2u; ++yy)
    for (uint xx = 0u; xx < 2u; ++xx)
    {
        const float2 uv = (coord * 2 + int2(xx, yy) + 0.5f.xx) * texel_size;
        const float3 view_dir = DecodeOctaUv(uv);

        float3 irrad, trans;
        Fog_SampleStatic(pk_SceneEnv_Origin.xyz, view_dir, PK_STATIC_FOG_VIRTUAL_DISTANCE, irrad, trans);

        local_irrad += irrad;
        local_trans += trans;

        float3 color = pk_SceneEnv.SampleLevel(pk_SamplerBilinearMirror, uv, 0.0f).rgb * pk_SceneEnv_Exposure;
        color = lerp(irrad, color, trans);
        pk_Image[coord * 2 + int2(xx, yy)] = EncodeE5BGR9(color);
    }

    // Mip 1
    {
        local_irrad /= 4;
        local_trans /= 4;

        float3 color = pk_SceneEnv.SampleLevel(pk_SamplerBilinearMirror, uv_base, 1.0f).rgb * pk_SceneEnv_Exposure;
        color = lerp(local_irrad, color, local_trans);
        pk_Image1[coord] = EncodeE5BGR9(color);
    }

    // Mip 2
    {
        const uint swap_id_v = QuadSwapIdVertical8x8(gl_SubgroupInvocationID);
        const uint swap_id_h = QuadSwapIdHorizontal(gl_SubgroupInvocationID);
        
        const float3 irrad_v = WaveReadLaneAt(local_irrad, swap_id_v);
        const float3 trans_v = WaveReadLaneAt(local_trans, swap_id_v);
        local_irrad += irrad_v;
        local_trans += trans_v;

        const float3 irrad_h = WaveReadLaneAt(local_irrad, swap_id_h);
        const float3 trans_h = WaveReadLaneAt(local_trans, swap_id_h);
        local_irrad += irrad_h;
        local_trans += trans_h;

        local_irrad /= 4;
        local_trans /= 4;

        float3 color = pk_SceneEnv.SampleLevel(pk_SamplerBilinearMirror, uv_base, 2.0f).rgb * pk_SceneEnv_Exposure;
        color = lerp(local_irrad, color, local_trans);

        if ((thread & 0x9u) == 0u)
        {
            pk_Image2[coord >> 1] = EncodeE5BGR9(color);
        }
    }

    // Mip 3
    {
        const uint2 wave_coord = uint2(gl_SubgroupInvocationID % 8u, gl_SubgroupInvocationID / 8u);
        const uint2 wave_coord_base = wave_coord / 2;
        const uint2 wave_coord_swap = (wave_coord_base / 2) * 4 + ((wave_coord_base + 1) % 2) * 2;

        const uint swap_id_v = wave_coord_swap.y * 8 + wave_coord.x;
        const uint swap_id_h = wave_coord.y * 8 + wave_coord_swap.x;

        const float3 irrad_v = WaveReadLaneAt(local_irrad, swap_id_v);
        const float3 trans_v = WaveReadLaneAt(local_trans, swap_id_v);
        local_irrad += irrad_v;
        local_trans += trans_v;

        const float3 irrad_h = WaveReadLaneAt(local_irrad, swap_id_h);
        const float3 trans_h = WaveReadLaneAt(local_trans, swap_id_h);
        local_irrad += irrad_h;
        local_trans += trans_h;
        
        local_irrad /= 4;
        local_trans /= 4;

        float3 color = pk_SceneEnv.SampleLevel(pk_SamplerBilinearMirror, uv_base, 3.0f).rgb * pk_SceneEnv_Exposure;
        color = lerp(local_irrad, color, local_trans);

        if ((thread & 0x1Bu) == 0u)
        {
            pk_Image3[coord >> 2] = EncodeE5BGR9(color);
        }
    }

    // Combine subgroups
    {
        local_irrad = WaveActiveSum(local_irrad);
        local_trans = WaveActiveSum(local_trans);
        local_irrad /= gl_SubgroupSize;
        local_trans /= gl_SubgroupSize;

        if (gl_SubgroupInvocationID == 0 && gl_SubgroupID == 1)
        {
            lds_irrad = local_irrad;
            lds_trans = local_trans;
        }
    }

    GroupMemoryBarrierWithGroupSync();

    // Mip 4
    if (thread == 0u)
    {
        local_irrad = (local_irrad + lds_irrad) / 2;
        local_trans = (local_trans + lds_trans) / 2;

        float3 color = pk_SceneEnv.SampleLevel(pk_SamplerBilinearMirror, uv_base, 4.0f).rgb * pk_SceneEnv_Exposure;
        color = lerp(local_irrad, color, local_trans);

        pk_Image4[coord >> 3] = EncodeE5BGR9(color);
    }
}

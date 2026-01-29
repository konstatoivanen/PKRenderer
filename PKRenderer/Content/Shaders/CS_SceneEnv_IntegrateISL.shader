
#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_shader_subgroup_shuffle : enable
#pragma pk_program SHADER_STAGE_COMPUTE IntegrateCs
#pragma pk_multi_compile PASS_INTEGRATE PASS_REDUCE_MIP
#define PK_USE_SINGLE_DESCRIPTOR_SET
#include "includes/Common.glsl"
#include "includes/ComputeQuadSwap.glsl"
#include "includes/SceneEnv.glsl"
#include "includes/Encoding.glsl"
#include "includes/BRDF.glsl"
#include "includes/Noise.glsl"

PK_DECLARE_SET_DRAW uniform writeonly restrict uimage2D pk_Image;
PK_DECLARE_SET_DRAW uniform writeonly restrict uimage2D pk_Image1;
PK_DECLARE_SET_DRAW uniform writeonly restrict uimage2D pk_Image2;
PK_DECLARE_SET_DRAW uniform writeonly restrict uimage2D pk_Image3;

shared float3 lds_scatter;

float3 GetDirectionSphere(float2 xi)
{
    const float u1 = 2 * xi.x - 1;
    const float u2 = 2 * xi.y - 1;
    const float d = 1 - (abs(u1) + abs(u2));
    const float r = 1 - abs(d);
    const float phi = r == 0 ? 0 : PK_PI / 4 * ((abs(u2) - abs(u1)) / r + 1);
    const float f = r * sqrt(2 - r * r);
    return float3(f * sign(u1) * cos(phi), f * sign(u2) * sin(phi), sign(d) * (1 - r * r));
}

float3 GetDirectionHemisphere(float2 xi)
{
    float u1 = xi.x;
    float u2 = xi.y;
    float z = u1;
    float r = sqrt(max(0.0f, 1.0f - z * z));
    float phi = PK_TWO_PI * u2;
    float x = r * cos(phi);
    float y = r * sin(phi);
    return float3(x, y, z);
}

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void IntegrateCs()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const uint thread = gl_LocalInvocationIndex;

    float3 local_scatter = 0.0f.xxx;

#if defined(PASS_INTEGRATE)
    const uint sample_count = 20000u;
    const float2 texel_size = 1.0f.xx / imageSize(pk_Image).xy;
    const float3 local_dir = DecodeOctaUv((coord + 0.5f.xx) * texel_size);

    for (uint i = 0u; i < sample_count; ++i)
    {
        const float2 xi = Hammersley(i, sample_count);
        const float3 sample_dir = GetDirectionSphere(xi);
 
        const float2 sample_uv = EncodeOctaUv(sample_dir);
        const float3 sample_scatter = textureLod(pk_SceneEnv, sample_uv, 0.0f).rgb * pk_SceneEnv_Exposure;

        const float sample_cosa = dot(local_dir, sample_dir);
        const float sample_phase = Fp_HenyeyGreensteinDual(sample_cosa, pk_Fog_Phase0, pk_Fog_Phase1, pk_Fog_PhaseW);

        local_scatter += sample_scatter * sample_phase;
    }
    local_scatter *= PK_FOUR_PI;
    local_scatter /= sample_count;
#else
    const float2 texel_size = 1.0f.xx / textureSize(pk_SceneEnv_ISL, 4).xy;
    local_scatter = texture(pk_SceneEnv_ISL, (coord + 0.5f) * texel_size).rgb;
#endif

    imageStore(pk_Image, coord, uint4(EncodeE5BGR9(local_scatter)));

    // Mip 1
    {
        const uint swap_id_v = QuadSwapIdVertical8x8(gl_SubgroupInvocationID);
        const uint swap_id_h = QuadSwapIdHorizontal(gl_SubgroupInvocationID);
        
        const float3 scatter_v = subgroupShuffle(local_scatter, swap_id_v);
        local_scatter += scatter_v;

        const float3 scatter_h = subgroupShuffle(local_scatter, swap_id_h);
        local_scatter += scatter_h;

        local_scatter /= 4;

        if ((thread & 0x9u) == 0u)
        {
            imageStore(pk_Image1, coord >> 1, uint4(EncodeE5BGR9(local_scatter)));
        }
    }

    subgroupBarrier();

    // Mip 3
    {
        const uint2 wave_coord = uint2(gl_SubgroupInvocationID % 8u, gl_SubgroupInvocationID / 8u);
        const uint2 wave_coord_base = wave_coord / 2;
        const uint2 wave_coord_swap = (wave_coord_base / 2) * 4 + ((wave_coord_base + 1) % 2) * 2;

        const uint swap_id_v = wave_coord_swap.y * 8 + wave_coord.x;
        const uint swap_id_h = wave_coord.y * 8 + wave_coord_swap.x;

        const float3 scatter_v = subgroupShuffle(local_scatter, swap_id_v);
        local_scatter += scatter_v;

        const float3 scatter_h = subgroupShuffle(local_scatter, swap_id_h);
        local_scatter += scatter_h;
        
        local_scatter /= 4;

        if ((thread & 0x1Bu) == 0u)
        {
            imageStore(pk_Image2, coord >> 2, uint4(EncodeE5BGR9(local_scatter)));
        }
    }

    subgroupBarrier();

    // Combine subgroups
    {
        local_scatter = subgroupAdd(local_scatter);
        local_scatter /= gl_SubgroupSize;

        if (gl_SubgroupInvocationID == 0 && gl_SubgroupID == 1)
        {
            lds_scatter = local_scatter;
        }
    }

    barrier();

    // Mip 4
    if (thread == 0u)
    {
        local_scatter = (local_scatter + lds_scatter) / 2;
        imageStore(pk_Image3, coord >> 3, uint4(EncodeE5BGR9(local_scatter)));
    }
}

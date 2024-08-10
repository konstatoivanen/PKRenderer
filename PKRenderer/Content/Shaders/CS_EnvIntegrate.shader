
#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_shader_subgroup_shuffle : enable
#pragma pk_program SHADER_STAGE_COMPUTE main
#define PK_USE_SINGLE_DESCRIPTOR_SET
#include "includes/Common.glsl"
#include "includes/ComputeQuadSwap.glsl"
#include "includes/SceneEnv.glsl"
#include "includes/VolumeFog.glsl"
#include "includes/Encoding.glsl"

layout(r32ui, set = PK_SET_DRAW) uniform writeonly restrict uimage2D pk_Image;
layout(r32ui, set = PK_SET_DRAW) uniform writeonly restrict uimage2D pk_Image1;
layout(r32ui, set = PK_SET_DRAW) uniform writeonly restrict uimage2D pk_Image2;
layout(r32ui, set = PK_SET_DRAW) uniform writeonly restrict uimage2D pk_Image3;
layout(r32ui, set = PK_SET_DRAW) uniform writeonly restrict uimage2D pk_Image4;

shared float3 lds_irrad;
shared float3 lds_trans;

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
void main()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const uint thread = gl_LocalInvocationIndex;
    const float2 texelSize = 1.0f.xx / textureSize(pk_SceneEnv, 0).xy;
    const float2 baseUv = (coord + 1.0f.xx) * texelSize;

    float3 local_irrad = 0.0f.xxx;
    float3 local_trans = 0.0f.xxx;

    for (uint yy = 0u; yy < 2u; ++yy)
    for (uint xx = 0u; xx < 2u; ++xx)
    {
        const float2 uv = (coord * 2 + int2(xx, yy) + 0.5f.xx) * texelSize;
        const float3 viewdir = OctaDecode(uv);

        float3 irrad, trans;
        VFog_GetSky(viewdir, irrad, trans);

        local_irrad += irrad;
        local_trans += trans;

        float3 color = textureLod(pk_SceneEnv, uv, 0.0f).rgb * pk_SceneEnv_Exposure;
        color = lerp(irrad, color, trans);
        imageStore(pk_Image, coord * 2 + int2(xx, yy), uint4(EncodeE5BGR9(color)));
    }

    // Mip 1
    {
        local_irrad /= 4;
        local_trans /= 4;

        float3 color = textureLod(pk_SceneEnv, baseUv, 1.0f).rgb * pk_SceneEnv_Exposure;
        color = lerp(local_irrad, color, local_trans);
        imageStore(pk_Image1, coord, uint4(EncodeE5BGR9(color)));
    }

    // Mip 2
    {
        const uint swapIdVertical = QuadSwapIdVertical8x8(gl_SubgroupInvocationID);
        const float3 vertical_irrad = subgroupShuffle(local_irrad, swapIdVertical);
        const float3 vertical_trans = subgroupShuffle(local_trans, swapIdVertical);
        local_irrad += vertical_irrad;
        local_trans += vertical_trans;

        const uint swapIdHorizontal = QuadSwapIdHorizontal(gl_SubgroupInvocationID);
        const float3 horizontal_irrad = subgroupShuffle(local_irrad, swapIdHorizontal);
        const float3 horizontal_trans = subgroupShuffle(local_trans, swapIdHorizontal);
        local_irrad += horizontal_irrad;
        local_trans += horizontal_trans;

        local_irrad /= 4;
        local_trans /= 4;

        float3 color = textureLod(pk_SceneEnv, baseUv, 2.0f).rgb * pk_SceneEnv_Exposure;
        color = lerp(local_irrad, color, local_trans);

        if ((thread & 0x9u) == 0u)
        {
            imageStore(pk_Image2, coord >> 1, uint4(EncodeE5BGR9(color)));
        }
    }

    subgroupBarrier();

    // Mip 3
    {
        const uint2 subgroupCoord = uint2(gl_SubgroupInvocationID % 8u, gl_SubgroupInvocationID / 8u);
        const uint2 subgroupBase = subgroupCoord / 2;
        const uint2 subgroupSwapped = (subgroupBase / 2) * 4 + ((subgroupBase + 1) % 2) * 2;

        const uint swapIdVertical = subgroupSwapped.y * 8 + subgroupCoord.x;
        const uint swapIdHorizontal = subgroupCoord.y * 8 + subgroupSwapped.x;

        const float3 vertical_irrad = subgroupShuffle(local_irrad, swapIdVertical);
        const float3 vertical_trans = subgroupShuffle(local_trans, swapIdVertical);
        local_irrad += vertical_irrad;
        local_trans += vertical_trans;

        const float3 horizontal_irrad = subgroupShuffle(local_irrad, swapIdHorizontal);
        const float3 horizontal_trans = subgroupShuffle(local_trans, swapIdHorizontal);
        local_irrad += horizontal_irrad;
        local_trans += horizontal_trans;
        
        local_irrad /= 4;
        local_trans /= 4;

        float3 color = textureLod(pk_SceneEnv, baseUv, 3.0f).rgb * pk_SceneEnv_Exposure;
        color = lerp(local_irrad, color, local_trans);

        if ((thread & 0x1Bu) == 0u)
        {
            imageStore(pk_Image3, coord >> 2, uint4(EncodeE5BGR9(color)));
        }
    }

    subgroupBarrier();

    // Combine subgroups
    {
        local_irrad = subgroupAdd(local_irrad);
        local_trans = subgroupAdd(local_trans);
        local_irrad /= gl_SubgroupSize;
        local_trans /= gl_SubgroupSize;

        if (gl_SubgroupInvocationID == 0 && gl_SubgroupID == 1)
        {
            lds_irrad = local_irrad;
            lds_trans = local_trans;
        }
    }

    barrier();

    // Mip 4
    if (thread == 0u)
    {
        local_irrad += lds_irrad;
        local_trans += lds_trans;
        local_irrad /= 2;
        local_trans /= 2;

        float3 color = textureLod(pk_SceneEnv, baseUv, 4.0f).rgb * pk_SceneEnv_Exposure;
        color = lerp(local_irrad, color, local_trans);

        imageStore(pk_Image4, coord >> 3, uint4(EncodeE5BGR9(color)));
    }

}
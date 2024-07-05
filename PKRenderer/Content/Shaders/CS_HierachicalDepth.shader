
#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_shader_subgroup_shuffle : enable
#pragma pk_multi_compile _ PK_HIZ_FINAL_PASS
#pragma pk_program SHADER_STAGE_COMPUTE main

#include "includes/GBuffers.glsl"
#include "includes/ComputeQuadSwap.glsl"

layout(r16f, set = PK_SET_DRAW) uniform writeonly restrict image2DArray pk_Image;
layout(r16f, set = PK_SET_DRAW) uniform writeonly restrict image2DArray pk_Image1;
layout(r16f, set = PK_SET_DRAW) uniform writeonly restrict image2DArray pk_Image2;
layout(r16f, set = PK_SET_DRAW) uniform writeonly restrict image2DArray pk_Image3;
layout(r16f, set = PK_SET_DRAW) uniform writeonly restrict image2DArray pk_Image4;

#define GROUP_SIZE 8u
shared float2 lds_Depth;

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE, local_size_z = 1) in;
void main()
{
    const uint2 size = uint2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const uint thread = gl_LocalInvocationIndex;

    float2 local_depth = 0.0f.xx;

    {
#if defined(PK_HIZ_FINAL_PASS)
        float4 depths[2];

        [[unroll]]
        for (int i = 0; i < 2; ++i)
        {
            depths[i] = float4
                (
                    SampleHiZ(coord * 2 + int2(0, 0), i, 4),
                    SampleHiZ(coord * 2 + int2(0, 1), i, 4),
                    SampleHiZ(coord * 2 + int2(1, 1), i, 4),
                    SampleHiZ(coord * 2 + int2(1, 0), i, 4)
                    );
        }

        local_depth.x = cmin(depths[0]);
        local_depth.y = cmax(depths[1]);
#else
        const float4 depths = GatherViewDepths((coord * 2 + 1.0f.xx) / size);

        [[unroll]]
        for (uint i = 0; i < 2; ++i)
        {
            imageStore(pk_Image, int3(coord * 2 + int2(0, 1), i), depths.xxxx);
            imageStore(pk_Image, int3(coord * 2 + int2(1, 1), i), depths.yyyy);
            imageStore(pk_Image, int3(coord * 2 + int2(1, 0), i), depths.zzzz);
            imageStore(pk_Image, int3(coord * 2 + int2(0, 0), i), depths.wwww);
        }

        local_depth.x = cmin(depths);
        local_depth.y = cmax(depths);
#endif

        imageStore(pk_Image1, int3(coord, 0), local_depth.xxxx);
        imageStore(pk_Image1, int3(coord, 1), local_depth.yyyy);
    }

    // Mip 2
    {
        const uint swapIdVertical = QuadSwapIdVertical8x8(gl_SubgroupInvocationID);
        const float2 vertical_depth = subgroupShuffle(local_depth, swapIdVertical);
        local_depth.x = min(local_depth.x, vertical_depth.x);
        local_depth.y = max(local_depth.y, vertical_depth.y);

        const uint swapIdHorizontal = QuadSwapIdHorizontal(gl_SubgroupInvocationID);
        const float2 horizontal_depth = subgroupShuffle(local_depth, swapIdHorizontal);
        local_depth.x = min(local_depth.x, horizontal_depth.x);
        local_depth.y = max(local_depth.y, horizontal_depth.y);

        if ((thread & 0x9u) == 0u)
        {
            imageStore(pk_Image2, int3(coord / 2, 0), local_depth.xxxx);
            imageStore(pk_Image2, int3(coord / 2, 1), local_depth.yyyy);
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

        const float2 vertical_depth = subgroupShuffle(local_depth, swapIdVertical);
        local_depth.x = min(local_depth.x, vertical_depth.x);
        local_depth.y = max(local_depth.y, vertical_depth.y);

        const float2 horizontal_depth = subgroupShuffle(local_depth, swapIdHorizontal);
        local_depth.x = min(local_depth.x, horizontal_depth.x);
        local_depth.y = max(local_depth.y, horizontal_depth.y);

        if ((thread & 0x1Bu) == 0u)
        {
            imageStore(pk_Image3, int3(coord / 4, 0), local_depth.xxxx);
            imageStore(pk_Image3, int3(coord / 4, 1), local_depth.yyyy);
        }
    }

    subgroupBarrier();

    // Combine subgroups
    {
        local_depth.x = subgroupMin(local_depth.x);
        local_depth.y = subgroupMax(local_depth.y);

        if (gl_SubgroupInvocationID == 0 && gl_SubgroupID == 1)
        {
            lds_Depth = local_depth;
        }
    }

    barrier();

    // Mip 4
    if (thread == 0u)
    {
        local_depth.x = min(local_depth.x, lds_Depth.x);
        local_depth.y = max(local_depth.y, lds_Depth.y);
        imageStore(pk_Image4, int3(coord / 8, 0), local_depth.xxxx);
        imageStore(pk_Image4, int3(coord / 8, 1), local_depth.yyyy);
    }
}
#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl

#multi_compile _ PK_HIZ_FINAL_PASS

layout(rg16f, set = PK_SET_SHADER) uniform writeonly restrict image2DArray _DestinationTex;
layout(rg16f, set = PK_SET_SHADER) uniform writeonly restrict image2DArray _DestinationMip1;
layout(rg16f, set = PK_SET_SHADER) uniform writeonly restrict image2DArray _DestinationMip2;
layout(rg16f, set = PK_SET_SHADER) uniform writeonly restrict image2DArray _DestinationMip3;
layout(rg16f, set = PK_SET_SHADER) uniform writeonly restrict image2DArray _DestinationMip4;

#define GROUP_SIZE 8u
shared float2 lds_Data[GROUP_SIZE * GROUP_SIZE];

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE, local_size_z = 1) in;
void main()
{
    const uint2 size = uint2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const uint thread = gl_LocalInvocationIndex;
    float2 local_depth = 0.0f.xx;

    {
#if defined(PK_HIZ_FINAL_PASS)
        const float min00 = SampleMinZ(coord * 2 + int2(0,0), 4);
        const float min01 = SampleMinZ(coord * 2 + int2(0,1), 4);
        const float min11 = SampleMinZ(coord * 2 + int2(1,1), 4);
        const float min10 = SampleMinZ(coord * 2 + int2(1,0), 4);

        const float max00 = SampleMaxZ(coord * 2 + int2(0, 0), 4);
        const float max01 = SampleMaxZ(coord * 2 + int2(0, 1), 4);
        const float max11 = SampleMaxZ(coord * 2 + int2(1, 1), 4);
        const float max10 = SampleMaxZ(coord * 2 + int2(1, 0), 4);

        local_depth.x = min(min(min(min00, min01), min11), min10);
        local_depth.y = max(max(max(max00, max01), max11), max10);
        lds_Data[thread] = local_depth;
#else
        const float4 depths = GatherLinearDepths((coord * 2 + 1.0f.xx) / size);
        local_depth.x = cmin(depths);
        local_depth.y = cmax(depths);
        lds_Data[thread] = local_depth;

        imageStore(_DestinationTex, int3(coord * 2 + int2(0,1), 0), depths.xxxx);
        imageStore(_DestinationTex, int3(coord * 2 + int2(1,1), 0), depths.yyyy);
        imageStore(_DestinationTex, int3(coord * 2 + int2(1,0), 0), depths.zzzz);
        imageStore(_DestinationTex, int3(coord * 2 + int2(0,0), 0), depths.wwww);

        imageStore(_DestinationTex, int3(coord * 2 + int2(0, 1), 1), depths.xxxx);
        imageStore(_DestinationTex, int3(coord * 2 + int2(1, 1), 1), depths.yyyy);
        imageStore(_DestinationTex, int3(coord * 2 + int2(1, 0), 1), depths.zzzz);
        imageStore(_DestinationTex, int3(coord * 2 + int2(0, 0), 1), depths.wwww);
#endif
        imageStore(_DestinationMip1, int3(coord, 0), local_depth.xxxx);
        imageStore(_DestinationMip1, int3(coord, 1), local_depth.yyyy);
    }
    barrier();

    if ((thread & 0x9u) == 0u)
    {
        const float2 depth1 = lds_Data[thread + 0x01u];
        const float2 depth2 = lds_Data[thread + 0x08u];
        const float2 depth3 = lds_Data[thread + 0x09u];
        local_depth.x = min(min(local_depth.x, depth1.x), min(depth2.x, depth3.x));
        local_depth.y = max(max(local_depth.y, depth1.y), max(depth2.y, depth3.y));
        lds_Data[thread] = local_depth;
        imageStore(_DestinationMip2, int3(coord / 2, 0), local_depth.xyxy);
        imageStore(_DestinationMip2, int3(coord / 2, 1), local_depth.xyxy);
    }
    barrier();

    if ((thread & 0x1Bu) == 0u)
    {
        float2 depth1 = lds_Data[thread + 0x02u];
        float2 depth2 = lds_Data[thread + 0x10u];
        float2 depth3 = lds_Data[thread + 0x12u];
        local_depth.x = min(min(local_depth.x, depth1.x), min(depth2.x, depth3.x));
        local_depth.y = max(max(local_depth.y, depth1.y), max(depth2.y, depth3.y));
        lds_Data[thread] = local_depth;
        imageStore(_DestinationMip3, int3(coord / 4, 0), local_depth.xyxy);
        imageStore(_DestinationMip3, int3(coord / 4, 1), local_depth.xyxy);
    }
    barrier();

    if (thread == 0u)
    {
        float2 depth1 = lds_Data[thread + 0x04u];
        float2 depth2 = lds_Data[thread + 0x20u];
        float2 depth3 = lds_Data[thread + 0x24u];
        local_depth.x = min(min(local_depth.x, depth1.x), min(depth2.x, depth3.x));
        local_depth.y = max(max(local_depth.y, depth1.y), max(depth2.y, depth3.y));
        imageStore(_DestinationMip4, int3(coord / 8, 0), local_depth.xyxy);
        imageStore(_DestinationMip4, int3(coord / 8, 1), local_depth.xyxy);
    }
    barrier();
}
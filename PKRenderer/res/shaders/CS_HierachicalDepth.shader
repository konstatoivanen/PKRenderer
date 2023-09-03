#version 460
#pragma PROGRAM_COMPUTE
#include includes/GBuffers.glsl
#include includes/SharedSceneGI.glsl

#multi_compile _ PK_HIZ_FINAL_PASS

layout(r16f, set = PK_SET_SHADER) uniform writeonly restrict image2DArray _DestinationTex;
layout(r16f, set = PK_SET_SHADER) uniform writeonly restrict image2DArray _DestinationMip1;
layout(r16f, set = PK_SET_SHADER) uniform writeonly restrict image2DArray _DestinationMip2;
layout(r16f, set = PK_SET_SHADER) uniform writeonly restrict image2DArray _DestinationMip3;
layout(r16f, set = PK_SET_SHADER) uniform writeonly restrict image2DArray _DestinationMip4;

#define GROUP_SIZE 8u
shared float lds_MinZ[GROUP_SIZE * GROUP_SIZE];
shared float lds_MaxZ[GROUP_SIZE * GROUP_SIZE];
shared float lds_AvgZ[GROUP_SIZE * GROUP_SIZE];

#define STORE_DEPTHS(tex, px, depth)          \
    imageStore(tex, int3(px, 0), depth.xxxx); \
    imageStore(tex, int3(px, 1), depth.yyyy); \
    imageStore(tex, int3(px, 2), depth.zzzz); \

#define STORE_DEPTHS_LDS(t, depth) \
    lds_MinZ[t] = depth.x;         \
    lds_MaxZ[t] = depth.y;         \
    lds_AvgZ[t] = depth.z;         \

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE, local_size_z = 1) in;
void main()
{
    const uint2 size = uint2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const uint thread = gl_LocalInvocationIndex;
    float3 local_depth = 0.0f.xxx;

    {
#if defined(PK_HIZ_FINAL_PASS)
        const float4 minz = float4
        (
            SampleMinZ(coord * 2 + int2(0, 0), 4),
            SampleMinZ(coord * 2 + int2(0, 1), 4),
            SampleMinZ(coord * 2 + int2(1, 1), 4),
            SampleMinZ(coord * 2 + int2(1, 0), 4)
        );

        const float4 maxz = float4
        (
            SampleMinZ(coord * 2 + int2(0, 0), 4),
            SampleMinZ(coord * 2 + int2(0, 1), 4),
            SampleMinZ(coord * 2 + int2(1, 1), 4),
            SampleMinZ(coord * 2 + int2(1, 0), 4)
        );

        const float4 avgz = float4
        (
            SampleAvgZ(coord * 2 + int2(0, 0), 4),
            SampleAvgZ(coord * 2 + int2(0, 1), 4),
            SampleAvgZ(coord * 2 + int2(1, 1), 4),
            SampleAvgZ(coord * 2 + int2(1, 0), 4)
        );

        local_depth.x = cmin(minz);
        local_depth.y = cmax(maxz);
        local_depth.z = dot(avgz, 0.25f.xxxx);
#else
        const float4 depths = GatherViewDepths((coord * 2 + 1.0f.xx) / size);
        local_depth.x = cmin(depths);
        local_depth.y = cmax(depths);
        local_depth.z = dot(depths, 0.25f.xxxx);

        #pragma unroll 3
        for (uint i = 0; i < 3; ++i)
        {
            imageStore(_DestinationTex, int3(coord * 2 + int2(0,1), i), depths.xxxx);
            imageStore(_DestinationTex, int3(coord * 2 + int2(1,1), i), depths.yyyy);
            imageStore(_DestinationTex, int3(coord * 2 + int2(1,0), i), depths.zzzz);
            imageStore(_DestinationTex, int3(coord * 2 + int2(0,0), i), depths.wwww);
        }
#endif

        STORE_DEPTHS_LDS(thread, local_depth)
        STORE_DEPTHS(_DestinationMip1, coord, local_depth)
    }
    barrier();

    if ((thread & 0x9u) == 0u)
    {
        const float4 minz = float4(local_depth.x, lds_MinZ[thread + 0x01u], lds_MinZ[thread + 0x08u], lds_MinZ[thread + 0x09u]);
        const float4 maxz = float4(local_depth.y, lds_MaxZ[thread + 0x01u], lds_MaxZ[thread + 0x08u], lds_MaxZ[thread + 0x09u]);
        const float4 avgz = float4(local_depth.z, lds_AvgZ[thread + 0x01u], lds_AvgZ[thread + 0x08u], lds_AvgZ[thread + 0x09u]);
        local_depth.x = cmin(minz);
        local_depth.y = cmax(maxz);
        local_depth.z = dot(avgz, 0.25f.xxxx);

        STORE_DEPTHS_LDS(thread, local_depth)
        STORE_DEPTHS(_DestinationMip2, coord / 2, local_depth)
    }
    barrier();

    if ((thread & 0x1Bu) == 0u)
    {
        const float4 minz = float4(local_depth.x, lds_MinZ[thread + 0x02u], lds_MinZ[thread + 0x10u], lds_MinZ[thread + 0x12u]);
        const float4 maxz = float4(local_depth.y, lds_MaxZ[thread + 0x02u], lds_MaxZ[thread + 0x10u], lds_MaxZ[thread + 0x12u]);
        const float4 avgz = float4(local_depth.z, lds_AvgZ[thread + 0x02u], lds_AvgZ[thread + 0x10u], lds_AvgZ[thread + 0x12u]);
        local_depth.x = cmin(minz);
        local_depth.y = cmax(maxz);
        local_depth.z = dot(avgz, 0.25f.xxxx);

        STORE_DEPTHS_LDS(thread, local_depth)
        STORE_DEPTHS(_DestinationMip3, coord / 4, local_depth)
    }
    barrier();

    if (thread == 0u)
    {
        const float4 minz = float4(local_depth.x, lds_MinZ[thread + 0x04u], lds_MinZ[thread + 0x20u], lds_MinZ[thread + 0x24u]);
        const float4 maxz = float4(local_depth.y, lds_MaxZ[thread + 0x04u], lds_MaxZ[thread + 0x20u], lds_MaxZ[thread + 0x24u]);
        const float4 avgz = float4(local_depth.z, lds_AvgZ[thread + 0x04u], lds_AvgZ[thread + 0x20u], lds_AvgZ[thread + 0x24u]);
        local_depth.x = cmin(minz);
        local_depth.y = cmax(maxz);
        local_depth.z = dot(avgz, 0.25f.xxxx);

        STORE_DEPTHS(_DestinationMip4, coord / 8, local_depth)
    }
}
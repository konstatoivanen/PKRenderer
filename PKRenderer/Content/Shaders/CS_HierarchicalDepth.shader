
#pragma pk_program Compute HierarchicalDepthCs

#include "includes/GBuffers.glsl"

// Backbuffer is 64px aligned. check alignment for mips 6-12
uniform RWTexture2DArray<float> pk_ImageArray[13];
uniform RWTexture2DArray<float> pk_Image6; // Load store for last mips.
uniform RWStructuredBuffer<uint> pk_HZB_WorkgroupCounter;
uniform uint4 pk_HZB_Parameters;

#define SpdFormat float2

groupshared float lds_Depth_Min[16][16];
groupshared float lds_Depth_Max[16][16];

float2 SpdReduce4(float2 v0, float2 v1, float2 v2, float2 v3)
{
    float2 r;
    r.x = min(min(min(v0.x, v1.x), v2.x), v3.x);
    r.y = max(max(max(v0.y, v1.y), v2.y), v3.y);
    return r;
}

float2 SpdReduce2(float2 v0, float2 v1)
{
    float2 r;
    r.x = min(v0.x, v1.x);
    r.y = max(v0.y, v1.y);
    return r;
}

float2 SpdLoadIntermediate(uint x, uint y) { return float2(lds_Depth_Min[x][y], lds_Depth_Max[x][y]);  }
void SpdStoreIntermediate(uint x, uint y, float2 value) { lds_Depth_Min[x][y] = value.x; lds_Depth_Max[x][y] = value.y; }
void SpdIncreaseAtomicCounter() { InterlockedAdd(pk_HZB_WorkgroupCounter[0], 1u); }
uint SpdGetAtomicCounter() {  return pk_HZB_WorkgroupCounter[0]; }

float2 SpdReduceLoadSourceImage4(int2 base)
{
    const float4 depths = GatherViewDepths((base + 1.0f.xx) / pk_View.ScreenSize.xy);

    // Copy source values to first image
    [[unroll]]
    for (uint i = 0; i < 2; ++i)
    {
        pk_ImageArray[0][int3(base + int2(0, 1), i)] = depths.x;
        pk_ImageArray[0][int3(base + int2(1, 1), i)] = depths.y;
        pk_ImageArray[0][int3(base + int2(1, 0), i)] = depths.z;
        pk_ImageArray[0][int3(base + int2(0, 0), i)] = depths.w;
    }

    return float2(cmin(depths), cmax(depths));
}

float2 SpdLoadMip6(int2 coord) { return float2(pk_Image6[int3(coord, 0)], pk_Image6[int3(coord, 1)]); }
void SpdStoreMip6(int2 coord, float2 v) { pk_Image6[int3(coord, 0)] = v.x; pk_Image6[int3(coord, 1)] = v.y; }
void SpdStoreMip(int mip, int2 coord, float2 v) { pk_ImageArray[mip][int3(coord, 0)] = v.x; pk_ImageArray[mip][int3(coord, 1)] = v.y; }

#define SPD_SUPPORT_NON_POT_6_TO_12

#include "includes/ffx_spd.glsl"

[numthreads(256, 1u, 1u)]
void HierarchicalDepthCs()
{
    SpdDownsample(gl_WorkGroupID.xy, gl_LocalInvocationIndex, pk_HZB_Parameters.x, pk_HZB_Parameters.y, pk_HZB_Parameters.zw);
}

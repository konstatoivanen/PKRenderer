
#pragma pk_program SHADER_STAGE_COMPUTE HierarchicalDepthCs

#include "includes/GBuffers.glsl"

// @TODO non pot textures are not supported.
// Backbuffer is 64px aligned. check alignment for mips 6-12

uniform writeonly restrict image2DArray pk_Image;
uniform writeonly restrict image2DArray pk_Image1;
uniform writeonly restrict image2DArray pk_Image2;
uniform writeonly restrict image2DArray pk_Image3;
uniform writeonly restrict image2DArray pk_Image4;
uniform writeonly restrict image2DArray pk_Image5;
uniform coherent image2DArray pk_Image6; // Load store for last mips.
uniform writeonly restrict image2DArray pk_Image7;
uniform writeonly restrict image2DArray pk_Image8;
uniform writeonly restrict image2DArray pk_Image9;
uniform writeonly restrict image2DArray pk_Image10;
uniform writeonly restrict image2DArray pk_Image11;
uniform writeonly restrict image2DArray pk_Image12;
uniform RWBuffer<uint, 1u> pk_WorkgroupCounter;
uniform uint2 pk_NumMipsAndWorkGroups;

#define SpdFormat float2

shared float lds_Depth_Min[16][16];
shared float lds_Depth_Max[16][16];

float2 SpdReduce4(float2 v0, float2 v1, float2 v2, float2 v3)
{
    float2 r;
    r.x = min(min(min(v0.x, v1.x), v2.x), v3.x);
    r.y = max(max(max(v0.y, v1.y), v2.y), v3.y);
    return r;
}

float2 SpdLoadIntermediate(uint x, uint y) { return float2(lds_Depth_Min[x][y], lds_Depth_Max[x][y]);  }
void SpdStoreIntermediate(uint x, uint y, float2 value) { lds_Depth_Min[x][y] = value.x; lds_Depth_Max[x][y] = value.y; }
void SpdIncreaseAtomicCounter() { atomicAdd(pk_WorkgroupCounter, 1u); }
uint SpdGetAtomicCounter() {  return pk_WorkgroupCounter; }

float2 SpdReduceLoadSourceImage4(int2 base)
{
    const float4 depths = GatherViewDepths((base + 1.0f.xx) / pk_ScreenSize.xy);

    // Copy source values to first image
    [[unroll]]
    for (uint i = 0; i < 2; ++i)
    {
        imageStore(pk_Image, int3(base + int2(0, 1), i), depths.xxxx);
        imageStore(pk_Image, int3(base + int2(1, 1), i), depths.yyyy);
        imageStore(pk_Image, int3(base + int2(1, 0), i), depths.zzzz);
        imageStore(pk_Image, int3(base + int2(0, 0), i), depths.wwww);
    }

    return float2(cmin(depths), cmax(depths));
}

float2 SpdLoadMip5(int2 coord) { return float2(imageLoad(pk_Image6, int3(coord, 0)).x, imageLoad(pk_Image6, int3(coord, 1)).x); }
void SpdStoreMip0(int2 coord, float2 v) { imageStore(pk_Image1, int3(coord, 0), v.xxxx); imageStore(pk_Image1, int3(coord, 1), v.yyyy); }
void SpdStoreMip1(int2 coord, float2 v) { imageStore(pk_Image2, int3(coord, 0), v.xxxx); imageStore(pk_Image2, int3(coord, 1), v.yyyy); }
void SpdStoreMip2(int2 coord, float2 v) { imageStore(pk_Image3, int3(coord, 0), v.xxxx); imageStore(pk_Image3, int3(coord, 1), v.yyyy); }
void SpdStoreMip3(int2 coord, float2 v) { imageStore(pk_Image4, int3(coord, 0), v.xxxx); imageStore(pk_Image4, int3(coord, 1), v.yyyy); }
void SpdStoreMip4(int2 coord, float2 v) { imageStore(pk_Image5, int3(coord, 0), v.xxxx); imageStore(pk_Image5, int3(coord, 1), v.yyyy); }
void SpdStoreMip5(int2 coord, float2 v) { imageStore(pk_Image6, int3(coord, 0), v.xxxx); imageStore(pk_Image6, int3(coord, 1), v.yyyy); }
void SpdStoreMip6(int2 coord, float2 v) { imageStore(pk_Image7, int3(coord, 0), v.xxxx); imageStore(pk_Image7, int3(coord, 1), v.yyyy); }
void SpdStoreMip7(int2 coord, float2 v) { imageStore(pk_Image8, int3(coord, 0), v.xxxx); imageStore(pk_Image8, int3(coord, 1), v.yyyy); }
void SpdStoreMip8(int2 coord, float2 v) { imageStore(pk_Image9, int3(coord, 0), v.xxxx); imageStore(pk_Image9, int3(coord, 1), v.yyyy); }
void SpdStoreMip9(int2 coord, float2 v) { imageStore(pk_Image10, int3(coord, 0), v.xxxx); imageStore(pk_Image10, int3(coord, 1), v.yyyy); }
void SpdStoreMip10(int2 coord, float2 v) { imageStore(pk_Image11, int3(coord, 0), v.xxxx); imageStore(pk_Image11, int3(coord, 1), v.yyyy); }
void SpdStoreMip11(int2 coord, float2 v) { imageStore(pk_Image12, int3(coord, 0), v.xxxx); imageStore(pk_Image12, int3(coord, 1), v.yyyy); }

#include "includes/ffx_spd.glsl"

[pk_numthreads(256, 1u, 1u)]
void HierarchicalDepthCs()
{
    SpdDownsample(gl_WorkGroupID.xy, gl_LocalInvocationIndex, pk_NumMipsAndWorkGroups.x, pk_NumMipsAndWorkGroups.y);
}

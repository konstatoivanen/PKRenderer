#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl

layout(rg32ui, set = PK_SET_SHADER) uniform writeonly restrict uimage2DArray pk_GI_ScreenDataMip1;
layout(rg32ui, set = PK_SET_SHADER) uniform writeonly restrict uimage2DArray pk_GI_ScreenDataMip2;
layout(rg32ui, set = PK_SET_SHADER) uniform writeonly restrict uimage2DArray pk_GI_ScreenDataMip3;
layout(rg32ui, set = PK_SET_SHADER) uniform writeonly restrict uimage2DArray pk_GI_ScreenDataMip4;

#define GROUP_SIZE 8u
shared uint4 lds_Data[GROUP_SIZE * GROUP_SIZE];

void CombineSamplesDiff(inout GISampleDiff o, const uint4 s)
{
    const GISampleDiff u = GI_Unpack_SampleDiff(s);
    o.sh.Y += u.sh.Y;
    o.sh.CoCg += u.sh.CoCg;
    o.ao += u.ao;
    o.depth += u.depth;
}

void CombineSamplesSpec(inout GISampleSpec o, const uint2 s)
{
    const GISampleSpec u = GI_Unpack_SampleSpec(s);
    o.radiance += u.radiance;
    o.ao += u.ao;
    o.depth += u.depth;
}

uint4 CombinePackedDiff(const uint4 u0, const uint4 u1, const uint4 u2, const uint4 u3)
{
    GISampleDiff flt = pk_Zero_GISampleDiff;
    uint sum = 0u;

    if (u0.w != 0u) { CombineSamplesDiff(flt, u0); ++sum; }
    if (u0.w != 0u) { CombineSamplesDiff(flt, u1); ++sum; }
    if (u0.w != 0u) { CombineSamplesDiff(flt, u2); ++sum; }
    if (u0.w != 0u) { CombineSamplesDiff(flt, u3); ++sum; }

    if (sum == 0u)
    {
        return uint4(0u);
    }

    const float w = 1.0f / float(sum);
    flt.sh.Y *= w;
    flt.sh.CoCg *= w;
    flt.ao *= w;
    flt.depth *= w;
    return GI_Pack_SampleDiff(flt);
}

uint2 CombinePackedSpec(const uint2 u0, const uint2 u1, const uint2 u2, const uint2 u3)
{
    GISampleSpec flt = pk_Zero_GISampleSpec;
    uint sum = 0u;

    if (u0.y != 0u) { CombineSamplesSpec(flt, u0); ++sum; }
    if (u0.y != 0u) { CombineSamplesSpec(flt, u1); ++sum; }
    if (u0.y != 0u) { CombineSamplesSpec(flt, u2); ++sum; }
    if (u0.y != 0u) { CombineSamplesSpec(flt, u3); ++sum; }

    if (sum == 0u)
    {
        return uint2(0u);
    }

    const float w = 1.0f / float(sum);
    flt.radiance *= w;
    flt.ao *= w;
    flt.depth *= w;
    return GI_Pack_SampleSpec(flt);
}

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE, local_size_z = 1) in;
void main()
{
    const uint2 size = uint2(pk_ScreenSize.xy);
    const uint2 coord = gl_GlobalInvocationID.xy;
    const uint thread = gl_LocalInvocationIndex;

    uint2 baseCoords[4] =
    {
        coord * 2 + uint2(0, 0),
        coord * 2 + uint2(1, 0),
        coord * 2 + uint2(1, 1),
        coord * 2 + uint2(0, 1)
    };

    bool4 isOOB;
    isOOB.x = Any_Greater(baseCoords[0], size);
    isOOB.y = Any_Greater(baseCoords[1], size);
    isOOB.z = Any_Greater(baseCoords[2], size);
    isOOB.w = Any_Greater(baseCoords[3], size);
    
    uint4 packedDiff = uint4(0u);

    //----------DIFFUSE MIP----------//
    {
        uint4 packed0 = isOOB.x ? uint4(0u) : GI_Load_Packed_SampleDiff(int2(baseCoords[0]));
        uint4 packed1 = isOOB.y ? uint4(0u) : GI_Load_Packed_SampleDiff(int2(baseCoords[1]));
        uint4 packed2 = isOOB.z ? uint4(0u) : GI_Load_Packed_SampleDiff(int2(baseCoords[2]));
        uint4 packed3 = isOOB.w ? uint4(0u) : GI_Load_Packed_SampleDiff(int2(baseCoords[3]));
        lds_Data[thread] = packedDiff = CombinePackedDiff(packed0, packed1, packed2, packed3);
        imageStore(pk_GI_ScreenDataMip1, int3(coord, PK_GI_LVL_DIFF0), packedDiff.xyxy);
        imageStore(pk_GI_ScreenDataMip1, int3(coord, PK_GI_LVL_DIFF1), packedDiff.zwzw);
    }
    barrier();

    if ((thread & 0x9u) == 0u)
    {
        lds_Data[thread] = packedDiff = CombinePackedDiff(packedDiff, lds_Data[thread + 0x01u], lds_Data[thread + 0x08u], lds_Data[thread + 0x09u]);
        imageStore(pk_GI_ScreenDataMip2, int3(coord / 2u, PK_GI_LVL_DIFF0), packedDiff.xyxy);
        imageStore(pk_GI_ScreenDataMip2, int3(coord / 2u, PK_GI_LVL_DIFF1), packedDiff.zwzw);
    }
    barrier();

    if ((thread & 0x1Bu) == 0u)
    {
        lds_Data[thread] = packedDiff = CombinePackedDiff(packedDiff, lds_Data[thread + 0x02u], lds_Data[thread + 0x10u], lds_Data[thread + 0x12u]);
        imageStore(pk_GI_ScreenDataMip3, int3(coord / 4u, PK_GI_LVL_DIFF0), packedDiff.xyxy);
        imageStore(pk_GI_ScreenDataMip3, int3(coord / 4u, PK_GI_LVL_DIFF1), packedDiff.zwzw);
    }
    barrier();

    if (thread == 0u)
    {
        packedDiff = CombinePackedDiff(packedDiff, lds_Data[thread + 0x04u], lds_Data[thread + 0x20u], lds_Data[thread + 0x24u]);
        imageStore(pk_GI_ScreenDataMip4, int3(coord / 8u, PK_GI_LVL_DIFF0), packedDiff.xyxy);
        imageStore(pk_GI_ScreenDataMip4, int3(coord / 8u, PK_GI_LVL_DIFF1), packedDiff.zwzw);
    }
    barrier();


    //----------SPEC MIP----------//
    uint2 packedSpec = uint2(0u);
    
    {
        uint2 packed0 = isOOB.x ? uint2(0u) : GI_Load_Packed_SampleSpec(int2(baseCoords[0]));
        uint2 packed1 = isOOB.y ? uint2(0u) : GI_Load_Packed_SampleSpec(int2(baseCoords[1]));
        uint2 packed2 = isOOB.z ? uint2(0u) : GI_Load_Packed_SampleSpec(int2(baseCoords[2]));
        uint2 packed3 = isOOB.w ? uint2(0u) : GI_Load_Packed_SampleSpec(int2(baseCoords[3]));
        lds_Data[thread].xy = packedSpec = CombinePackedSpec(packed0, packed1, packed2, packed3);
        imageStore(pk_GI_ScreenDataMip1, int3(coord, PK_GI_LVL_SPEC), packedSpec.xyxy);
    }
    barrier();

    if ((thread & 0x9u) == 0u)
    {
        lds_Data[thread].xy = packedSpec = CombinePackedSpec(packedSpec, lds_Data[thread + 0x01u].xy, lds_Data[thread + 0x08u].xy, lds_Data[thread + 0x09u].xy);
        imageStore(pk_GI_ScreenDataMip2, int3(coord / 2u, PK_GI_LVL_SPEC), packedSpec.xyxy);
    }
    barrier();

    if ((thread & 0x1Bu) == 0u)
    {
        lds_Data[thread].xy = packedSpec = CombinePackedSpec(packedSpec, lds_Data[thread + 0x02u].xy, lds_Data[thread + 0x10u].xy, lds_Data[thread + 0x12u].xy);
        imageStore(pk_GI_ScreenDataMip3, int3(coord / 4u, PK_GI_LVL_SPEC), packedSpec.xyxy);
    }
    barrier();

    if (thread == 0u)
    {
        lds_Data[thread].xy = packedSpec = CombinePackedSpec(packedSpec, lds_Data[thread + 0x04u].xy, lds_Data[thread + 0x20u].xy, lds_Data[thread + 0x24u].xy);
        imageStore(pk_GI_ScreenDataMip4, int3(coord / 8u, PK_GI_LVL_SPEC), packedSpec.xyxy);
    }
}
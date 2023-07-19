#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/CTASWizzling.glsl

layout(rg32ui, set = PK_SET_SHADER) uniform writeonly restrict uimage2DArray _DestinationMip1;
layout(rg32ui, set = PK_SET_SHADER) uniform writeonly restrict uimage2DArray _DestinationMip2;
layout(rg32ui, set = PK_SET_SHADER) uniform writeonly restrict uimage2DArray _DestinationMip3;
layout(rg32ui, set = PK_SET_SHADER) uniform writeonly restrict uimage2DArray _DestinationMip4;

#define GROUP_SIZE 8u
shared uint4 lds_Diff[GROUP_SIZE * GROUP_SIZE];
shared uint2 lds_Spec[GROUP_SIZE * GROUP_SIZE];

uint4 CombinePackedDiff(const uint4 u0, const uint4 u1, const uint4 u2, const uint4 u3)
{
    GIDiff o = pk_Zero_GIDiff;
    const uint4 mask = uint4(u0.w != 0u, u1.w != 0u, u2.w != 0u, u3.w != 0u);
    const float w = 1.0f / float(max(mask.x + mask.y + mask.z + mask.w, 1u));

    const GIDiff s[4] =
    {
        GI_Unpack_Diff(u0),
        GI_Unpack_Diff(u1),
        GI_Unpack_Diff(u2),
        GI_Unpack_Diff(u3)
    };

    #pragma unroll 4
    for (uint i = 0u; i < 4; ++i)
    {
        o = GI_Sum_NoHistory(o, s[i], mask[i]);
    }

    return GI_Pack_Diff(GI_Mul_NoHistory(o, w));
}

uint2 CombinePackedSpec(const uint2 u0, const uint2 u1, const uint2 u2, const uint2 u3)
{
    GISpec o = pk_Zero_GISpec;
    const uint4 mask = uint4(u0.y != 0u, u1.y != 0u, u2.y != 0u, u3.y != 0u);
    const float w = 1.0f / float(max(mask.x + mask.y + mask.z + mask.w, 1u));

    const GISpec s[4] =
    {
        GI_Unpack_Spec(u0),
        GI_Unpack_Spec(u1),
        GI_Unpack_Spec(u2),
        GI_Unpack_Spec(u3)
    };

    for (uint i = 0u; i < 4; ++i)
    {
        o = GI_Sum_NoHistory(o, s[i], mask[i]);
    }

    return GI_Pack_Spec(GI_Mul_NoHistory(o, w));
}

uint2 GetSwizzledThreadID()
{
    return ThreadGroupTilingX
    (
        gl_NumWorkGroups.xy,
        uint2(GROUP_SIZE, GROUP_SIZE),
        8u,
        gl_LocalInvocationID.xy,
        gl_WorkGroupID.xy
    );
}

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE, local_size_z = 1) in;
void main()
{
    const uint2 size = uint2(pk_ScreenSize.xy);
    const uint2 coord = GetSwizzledThreadID();
    const uint thread = gl_LocalInvocationIndex;

    uint2 baseCoords[4] =
    {
        coord * 2 + uint2(0, 0),
        coord * 2 + uint2(1, 0),
        coord * 2 + uint2(1, 1),
        coord * 2 + uint2(0, 1)
    };

    uint4 packedDiff = uint4(0u);
    uint2 packedSpec = uint2(0u);

    //----------DIFFUSE MIP----------//
    {
        uint4 s_diff0 = GI_Load_Packed_Diff(int2(baseCoords[0]));
        uint4 s_diff1 = GI_Load_Packed_Diff(int2(baseCoords[1]));
        uint4 s_diff2 = GI_Load_Packed_Diff(int2(baseCoords[2]));
        uint4 s_diff3 = GI_Load_Packed_Diff(int2(baseCoords[3]));

        uint2 s_spec0 = GI_Load_Packed_Spec(int2(baseCoords[0]));
        uint2 s_spec1 = GI_Load_Packed_Spec(int2(baseCoords[1]));
        uint2 s_spec2 = GI_Load_Packed_Spec(int2(baseCoords[2]));
        uint2 s_spec3 = GI_Load_Packed_Spec(int2(baseCoords[3]));

        lds_Diff[thread] = packedDiff = CombinePackedDiff(s_diff0, s_diff1, s_diff2, s_diff3);
        lds_Spec[thread].xy = packedSpec = CombinePackedSpec(s_spec0, s_spec1, s_spec2, s_spec3);

        imageStore(_DestinationMip1, int3(coord, PK_GI_LVL_DIFF0), packedDiff.xyxy);
        imageStore(_DestinationMip1, int3(coord, PK_GI_LVL_DIFF1), packedDiff.zwzw);
        imageStore(_DestinationMip1, int3(coord, PK_GI_LVL_SPEC), packedSpec.xyxy);
    }

    barrier();

    if ((thread & 0x9u) == 0u)
    {
        lds_Diff[thread] = packedDiff = CombinePackedDiff(packedDiff, lds_Diff[thread + 0x01u], lds_Diff[thread + 0x08u], lds_Diff[thread + 0x09u]);
        lds_Spec[thread] = packedSpec = CombinePackedSpec(packedSpec, lds_Spec[thread + 0x01u], lds_Spec[thread + 0x08u], lds_Spec[thread + 0x09u]);
        imageStore(_DestinationMip2, int3(coord / 2u, PK_GI_LVL_DIFF0), packedDiff.xyxy);
        imageStore(_DestinationMip2, int3(coord / 2u, PK_GI_LVL_DIFF1), packedDiff.zwzw);
        imageStore(_DestinationMip2, int3(coord / 2u, PK_GI_LVL_SPEC), packedSpec.xyxy);
    }
    barrier();

    if ((thread & 0x1Bu) == 0u)
    {
        lds_Diff[thread] = packedDiff = CombinePackedDiff(packedDiff, lds_Diff[thread + 0x02u], lds_Diff[thread + 0x10u], lds_Diff[thread + 0x12u]);
        lds_Spec[thread] = packedSpec = CombinePackedSpec(packedSpec, lds_Spec[thread + 0x02u], lds_Spec[thread + 0x10u], lds_Spec[thread + 0x12u]);
        imageStore(_DestinationMip3, int3(coord / 4u, PK_GI_LVL_DIFF0), packedDiff.xyxy);
        imageStore(_DestinationMip3, int3(coord / 4u, PK_GI_LVL_DIFF1), packedDiff.zwzw);
        imageStore(_DestinationMip3, int3(coord / 4u, PK_GI_LVL_SPEC), packedSpec.xyxy);
    }
    barrier();

    if (thread == 0u)
    {
        packedDiff = CombinePackedDiff(packedDiff, lds_Diff[thread + 0x04u], lds_Diff[thread + 0x20u], lds_Diff[thread + 0x24u]);
        packedSpec = CombinePackedSpec(packedSpec, lds_Spec[thread + 0x04u], lds_Spec[thread + 0x20u], lds_Spec[thread + 0x24u]);
        imageStore(_DestinationMip4, int3(coord / 8u, PK_GI_LVL_DIFF0), packedDiff.xyxy);
        imageStore(_DestinationMip4, int3(coord / 8u, PK_GI_LVL_DIFF1), packedDiff.zwzw);
        imageStore(_DestinationMip4, int3(coord / 8u, PK_GI_LVL_SPEC), packedSpec.xyxy);
    }
}
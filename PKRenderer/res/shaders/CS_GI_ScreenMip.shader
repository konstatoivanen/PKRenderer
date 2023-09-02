#version 460
#extension GL_EXT_shader_atomic_float : enable
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

shared float lds_Diff_Mom1;
shared float lds_Diff_Mom2;
shared float lds_Spec_Mom1;
shared float lds_Spec_Mom2;

uint4 CombinePackedDiff(const uint4 u0, const uint4 u1, const uint4 u2, const uint4 u3)
{
    GIDiff o = pk_Zero_GIDiff;
    const byte4 mask = byte4(u0.w != 0u, u1.w != 0u, u2.w != 0u, u3.w != 0u);
    const float w = 1.0f / float(max(mask.x + mask.y + mask.z + mask.w, 1u));

    const GIDiff s[4] =
    {
        GI_Unpack_Diff(u0),
        GI_Unpack_Diff(u1),
        GI_Unpack_Diff(u2),
        GI_Unpack_Diff(u3)
    };

    [[unroll]]
    for (uint i = 0u; i < 4; ++i)
    {
        o = GI_Sum_NoHistory(o, s[i], mask[i]);
    }

    return GI_Pack_Diff(GI_Mul_NoHistory(o, w));
}

uint2 CombinePackedSpec(const uint2 u0, const uint2 u1, const uint2 u2, const uint2 u3)
{
    GISpec o = pk_Zero_GISpec;
    const byte4 mask = byte4(u0.y != 0u, u1.y != 0u, u2.y != 0u, u3.y != 0u);
    const float w = 1.0f / float(max(mask.x + mask.y + mask.z + mask.w, 1u));

    const GISpec s[4] =
    {
        GI_Unpack_Spec(u0),
        GI_Unpack_Spec(u1),
        GI_Unpack_Spec(u2),
        GI_Unpack_Spec(u3)
    };

    [[unroll]]
    for (uint i = 0u; i < 4; ++i)
    {
        o = GI_Sum_NoHistory(o, s[i], mask[i]);
    }

    return GI_Pack_Spec(GI_Mul_NoHistory(o, w));
}

layout(local_size_x = GROUP_SIZE, local_size_y = GROUP_SIZE, local_size_z = 1) in;
void main()
{
    const uint2 coord = GetXTiledThreadID(GROUP_SIZE, GROUP_SIZE, 8u);
    const uint thread = gl_LocalInvocationIndex;
    uint4 packedDiff = uint4(0u);
    uint2 packedSpec = uint2(0u);
    bool doCompute = imageLoad(pk_GI_ScreenDataMipMask, int2(coord / 8u)).x != 0u;

    // Coherent for entire workgroup. barriers should behave as expected.
    if (!doCompute)
    {
        return;
    }

    {
        // Anti Firefly
        if (gl_LocalInvocationIndex == 0)
        {
            lds_Diff_Mom1 = 0.0f;
            lds_Diff_Mom2 = 0.0f;
            lds_Spec_Mom1 = 0.0f;
            lds_Spec_Mom2 = 0.0f;
        }

        const int2 baseCoords[4] =
        {
            int2(coord) * 2 + int2(0, 0),
            int2(coord) * 2 + int2(1, 0),
            int2(coord) * 2 + int2(1, 1),
            int2(coord) * 2 + int2(0, 1)
        };

        const uint4 sp_diff[4] =
        {
            GI_Load_Packed_Diff(baseCoords[0]),
            GI_Load_Packed_Diff(baseCoords[1]),
            GI_Load_Packed_Diff(baseCoords[2]),
            GI_Load_Packed_Diff(baseCoords[3])
        };

        const uint2 sp_spec[4] =
        {
            GI_Load_Packed_Spec(baseCoords[0]),
            GI_Load_Packed_Spec(baseCoords[1]),
            GI_Load_Packed_Spec(baseCoords[2]),
            GI_Load_Packed_Spec(baseCoords[3])
        };

        const byte4 maskDiff = byte4(sp_diff[0].w != 0u, sp_diff[1].w != 0u, sp_diff[2].w != 0u, sp_diff[3].w != 0u);
        const byte4 maskSpec = byte4(sp_spec[0].y != 0u, sp_spec[1].y != 0u, sp_spec[2].y != 0u, sp_spec[3].y != 0u);
        const float wDiff = 1.0f / float(max(maskDiff.x + maskDiff.y + maskDiff.z + maskDiff.w, 1u));
        const float wSpec = 1.0f / float(max(maskSpec.x + maskSpec.y + maskSpec.z + maskSpec.w, 1u));

        GIDiff s_diff[4];
        GISpec s_spec[4];
        float4 lumaDiff, lumaSpec;

        [[unroll]]
        for (int i = 0; i < 4; ++i)
        {
            s_diff[i] = GI_Unpack_Diff(sp_diff[i]);
            s_spec[i] = GI_Unpack_Spec(sp_spec[i]);
            lumaDiff[i] = GI_Luminance(s_diff[i]);
            lumaSpec[i] = GI_Luminance(s_diff[i]);
        }
        
        barrier();
        atomicAdd(lds_Diff_Mom1, dot(lumaDiff, maskDiff) * wDiff);
        atomicAdd(lds_Diff_Mom2, dot(lumaDiff * lumaDiff, maskDiff) * wDiff);
        atomicAdd(lds_Spec_Mom1, dot(lumaSpec, maskSpec) * wSpec);
        atomicAdd(lds_Spec_Mom2, dot(lumaSpec * lumaSpec, maskSpec) * wSpec);
        barrier();

        const float2 momentsDiff = float2(lds_Diff_Mom1, lds_Diff_Mom2) / (GROUP_SIZE * GROUP_SIZE);
        const float2 momentsSpec = float2(lds_Spec_Mom1, lds_Spec_Mom2) / (GROUP_SIZE * GROUP_SIZE);
        lumaDiff = min(lumaDiff, momentsDiff.x + pow(abs(momentsDiff.y - pow2(momentsDiff.x)), 0.25f) * 2.5f);
        lumaSpec = min(lumaSpec, momentsSpec.x + pow(abs(momentsSpec.y - pow2(momentsSpec.x)), 0.25f) * 2.5f);

        GIDiff filteredDiff = pk_Zero_GIDiff;
        GISpec filteredSpec = pk_Zero_GISpec;

        [[unroll]]
        for (uint i = 0; i < 4; ++i)
        {
            float scaleDiff = (lumaDiff[i] + 1e-6f) / (GI_Luminance(s_diff[i]) + 1e-6f);
            float scaleSpec = (lumaSpec[i] + 1e-6f) / (GI_Luminance(s_spec[i]) + 1e-6f);
            s_diff[i].sh.Y *= scaleDiff;
            s_diff[i].sh.CoCg *= scaleDiff;
            s_spec[i].radiance *= scaleSpec;
            filteredDiff = GI_Sum_NoHistory(filteredDiff, s_diff[i], maskDiff[i]);
            filteredSpec = GI_Sum_NoHistory(filteredSpec, s_spec[i], maskSpec[i]);
        }

        packedDiff = GI_Pack_Diff(GI_Mul_NoHistory(filteredDiff, wDiff));
        packedSpec = GI_Pack_Spec(GI_Mul_NoHistory(filteredSpec, wSpec));

        lds_Diff[thread] = packedDiff;
        lds_Spec[thread] = packedSpec; 
        imageStore(_DestinationMip1, int3(coord, PK_GI_LVL_DIFF0), packedDiff.xyxy);
        imageStore(_DestinationMip1, int3(coord, PK_GI_LVL_DIFF1), packedDiff.zwzw);
        imageStore(_DestinationMip1, int3(coord, PK_GI_LVL_SPEC), packedSpec.xyxy);
    }

    barrier();

    [[branch]]
    if ((thread & 0x9u) == 0u)
    {
        lds_Diff[thread] = packedDiff = CombinePackedDiff(packedDiff, lds_Diff[thread + 0x01u], lds_Diff[thread + 0x08u], lds_Diff[thread + 0x09u]);
        lds_Spec[thread] = packedSpec = CombinePackedSpec(packedSpec, lds_Spec[thread + 0x01u], lds_Spec[thread + 0x08u], lds_Spec[thread + 0x09u]);
        imageStore(_DestinationMip2, int3(coord / 2u, PK_GI_LVL_DIFF0), packedDiff.xyxy);
        imageStore(_DestinationMip2, int3(coord / 2u, PK_GI_LVL_DIFF1), packedDiff.zwzw);
        imageStore(_DestinationMip2, int3(coord / 2u, PK_GI_LVL_SPEC), packedSpec.xyxy);
    }

    barrier();

    [[branch]]
    if ((thread & 0x1Bu) == 0u)
    {
        lds_Diff[thread] = packedDiff = CombinePackedDiff(packedDiff, lds_Diff[thread + 0x02u], lds_Diff[thread + 0x10u], lds_Diff[thread + 0x12u]);
        lds_Spec[thread] = packedSpec = CombinePackedSpec(packedSpec, lds_Spec[thread + 0x02u], lds_Spec[thread + 0x10u], lds_Spec[thread + 0x12u]);
        imageStore(_DestinationMip3, int3(coord / 4u, PK_GI_LVL_DIFF0), packedDiff.xyxy);
        imageStore(_DestinationMip3, int3(coord / 4u, PK_GI_LVL_DIFF1), packedDiff.zwzw);
        imageStore(_DestinationMip3, int3(coord / 4u, PK_GI_LVL_SPEC), packedSpec.xyxy);
    }

    barrier();

    [[branch]]
    if (thread == 0u)
    {
        packedDiff = CombinePackedDiff(packedDiff, lds_Diff[thread + 0x04u], lds_Diff[thread + 0x20u], lds_Diff[thread + 0x24u]);
        packedSpec = CombinePackedSpec(packedSpec, lds_Spec[thread + 0x04u], lds_Spec[thread + 0x20u], lds_Spec[thread + 0x24u]);
        imageStore(_DestinationMip4, int3(coord / 8u, PK_GI_LVL_DIFF0), packedDiff.xyxy);
        imageStore(_DestinationMip4, int3(coord / 8u, PK_GI_LVL_DIFF1), packedDiff.zwzw);
        imageStore(_DestinationMip4, int3(coord / 8u, PK_GI_LVL_SPEC), packedSpec.xyxy);
       
        imageStore(pk_GI_ScreenDataMipMask, int2(coord / 8u), uint4(0u));
    }
}
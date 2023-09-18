#version 460
#pragma PROGRAM_COMPUTE

#define PK_GI_LOAD_LVL 0
#define PK_GI_STORE_LVL 2

#include includes/SceneGIFiltering.glsl

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float4 normalRoughness = SampleWorldNormalRoughness(coord);

    uint4 packedDiff = GI_Load_Packed_Diff(coord);
    uint2 packedSpec = GI_Load_Packed_Spec(coord);
    const GIDiff accumDiff = GI_Unpack_Diff(packedDiff);
    const GISpec accumSpec = GI_Unpack_Spec(packedSpec);

    const float historyDiff = accumDiff.history;

#if PK_GI_APPROX_ROUGH_SPEC == 1
    // Rough spec might have invalid data.
    const float historySpec = lerp(PK_GI_SPEC_MAX_HISTORY, accumSpec.history, normalRoughness.w < PK_GI_MAX_ROUGH_SPEC);
#else
    const float historySpec = accumSpec.history;
#endif

    const float interpolant = min(historyDiff, historySpec) / PK_GI_HISTORY_FILL_THRESHOLD;
    const float level = min(3.0f - 1e-4f, 4.0f - 4.0f * interpolant);

    [[branch]]
    if (level > 0.0f)
    {
        const float depth = SampleMinZ(coord, 0);
        GIDiff diff = GIDiff(pk_ZeroSH, 0.0f, historyDiff);
        GISpec spec = GISpec(0.0f.xxx, 0.0f, historySpec);
        float wSum = 0.0f;

        GI_SF_HISTORY_MIP(coord, level, normalRoughness.xyz, depth, wSum, diff, spec)

            [[branch]]
        if (int(historyDiff) <= PK_GI_HISTORY_FILL_THRESHOLD && !Test_NaN_EPS6(wSum))
        {
            packedDiff = GI_Pack_Diff(GI_Mul_NoHistory(diff, 1.0f / wSum));
        }

        [[branch]]
        if (int(historySpec) <= PK_GI_HISTORY_FILL_THRESHOLD && !Test_NaN_EPS6(wSum))
        {
            spec = GI_Mul_NoHistory(spec, 1.0f / wSum);
            // Dont use history fill for smooth surfaces.
            spec.radiance = lerp(accumSpec.radiance, spec.radiance, smoothstep(0.1f, 0.6f, normalRoughness.w));
            packedSpec = GI_Pack_Spec(spec);
        }
    }

    GI_Store_Packed_Diff(coord, packedDiff);
    GI_Store_Packed_Spec(coord, packedSpec);
}

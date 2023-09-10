#version 460
#pragma PROGRAM_COMPUTE

#define PK_GI_LOAD_LVL 0
#define PK_GI_STORE_LVL 2

#include includes/SceneGIFiltering.glsl

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);

    uint4 packedDiff = GI_Load_Packed_Diff(coord);
    uint2 packedSpec = GI_Load_Packed_Spec(coord);

    const GIDiff accumDiff = GI_Unpack_Diff(packedDiff);
    const GISpec accumSpec = GI_Unpack_Spec(packedSpec);
    const float historyDiff = accumDiff.history;
    const float historySpec = accumSpec.history;
    const float level = min(3.0f - 1e-4f, 4.0f - min(historyDiff, historySpec));

    [[branch]]
    if (level > 0.0f)
    {
        const float depth = SampleMinZ(coord, 0);
        const float4 normalRoughness = SampleWorldNormalRoughness(coord);
        GIDiff diff = pk_Zero_GIDiff;
        GISpec spec = pk_Zero_GISpec;
        diff.history = historyDiff;
        spec.history = historySpec;
        float wSum = 0.0f;

        GI_SFLT_HISTORY_MIP(coord, level, normalRoughness.xyz, depth, wSum, diff, spec)

        [[branch]]
        if (int(historyDiff) <= 4 && !Test_NaN_EPS6(wSum))
        {
            packedDiff = GI_Pack_Diff(GI_Mul_NoHistory(diff, 1.0f / wSum));
        }

        [[branch]]
        if (int(historySpec) <= 4 && !Test_NaN_EPS6(wSum))
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

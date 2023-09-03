#version 460
#pragma PROGRAM_COMPUTE
#include includes/SceneGIFiltering.glsl

layout(local_size_x = PK_W_ALIGNMENT_16, local_size_y = PK_W_ALIGNMENT_4, local_size_z = 1) in;
void main()
{
    const int2 coord = int2(gl_GlobalInvocationID.xy);

    const float depth = SampleMinZ(coord, 0);

    if (!Test_DepthFar(depth))
    {
        return;
    }

    const float4 normalRoughness = SampleWorldNormalRoughness(coord);
    const GIDiff originalDiff = GI_Load_Diff(coord);
    const GISpec originalSpec = GI_Load_Spec(coord);
    const float historyDiff = originalDiff.history;
    const float historySpec = originalSpec.history;
    const float level = min(3.0f - 1e-4f, 4.0f - min(historyDiff, historySpec));

    if (level <= 0.0f)
    {
        return;
    }

    GIDiff diff = pk_Zero_GIDiff;
    GISpec spec = pk_Zero_GISpec;
    diff.history = historyDiff;
    spec.history = historySpec;
    float wSum = 0.0f;

    GI_SFLT_HISTORY_MIP(coord, level, normalRoughness.xyz, depth, wSum, diff, spec)

    if (int(historyDiff) <= 3 && !Test_NaN_EPS6(wSum))
    {
        GI_Store_Diff(coord, GI_Mul_NoHistory(diff, 1.0f / wSum));
    }

    if (int(historySpec) <= 3 && !Test_NaN_EPS6(wSum))
    {
        spec = GI_Mul_NoHistory(spec, 1.0f / wSum);
        // Dont use history fill for smooth surfaces.
        spec.radiance = lerp(originalSpec.radiance, spec.radiance, smoothstep(0.1f, 0.6f, normalRoughness.w));
        GI_Store_Spec(coord, spec);
    }
}

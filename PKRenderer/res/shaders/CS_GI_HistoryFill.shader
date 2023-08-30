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

    const float3 normal = SampleWorldNormal(coord);
    const float historyDiff = GI_Load_Diff(coord).history;
    const float historySpec = GI_Load_Spec(coord).history;
    const int iHistoryDiff = int(historyDiff);
    const int iHistorySpec = int(historySpec);

    const int mip = 3 - min(iHistoryDiff, iHistorySpec);

    if (mip < 0)
    {
        return;
    }

    GIDiff diff = pk_Zero_GIDiff;
    GISpec spec = pk_Zero_GISpec;
    diff.history = historyDiff;
    spec.history = historySpec;
    float wSum = 0.0f;

    GI_SFLT_HISTORY_FILL(coord, mip, normal, depth, wSum, diff, spec)

    if (iHistoryDiff <= 3 && !Test_NaN_EPS6(wSum))
    {
        GI_Store_Diff(coord, GI_Mul_NoHistory(diff, 1.0f / wSum));
    }

    if (iHistorySpec <= 3 && !Test_NaN_EPS6(wSum))
    {
        GI_Store_Spec(coord, GI_Mul_NoHistory(spec, 1.0f / wSum));
    }
}

#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl
#include includes/Kernels.glsl

layout(local_size_x = 16, local_size_y = 2, local_size_z = 1) in;
void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);

    if (Any_GEqual(coord, size))
    {
        return;
    }

    const float c_depth = SampleLinearDepth(coord);

    if (!Test_DepthFar(c_depth))
    {
        return;
    }

    SH c_diff = GI_Load_SH(coord, PK_GI_DIFF_LVL);
    SH c_spec = GI_Load_SH(coord, PK_GI_SPEC_LVL);

    const float4 c_nr = SampleWorldNormalRoughness(coord);
    const float3 c_normal = c_nr.xyz;
    const float c_roughness = c_nr.w;
    const float2 c_hisvar = GI_Load_HistoryVariance(coord);
    const float c_history = c_hisvar.x;
    const float c_variance = c_hisvar.y;
    const float c_lum = GI_SHToLuminance(c_diff, c_normal);

    const float wLumMult = c_history < 1.0f ? 0.0f : (1.0 / (100.0f * c_variance + 1e-4f));
    const float wRoughnessMult = saturate(c_roughness * 30);

    const float3 c_position = SampleWorldPosition(coord, size, c_depth);
    const float3x3 TBN = ComposeTBN(c_normal);
    const float2 rotation = make_rotation(pk_FrameIndex * (PK_PI / 3.0f));

    const float normalPower = clamp(c_history, 0.25f, 256.0f);
    const float scalePx = pk_ScreenParams.w * 2.0f / pk_MATRIX_P[1][1];
    const float scaleDepth = (pk_ProjectionParams.x + c_depth);
    const float scaleVariance = lerp(0.25f, 4.0f, c_variance * 100.0f);
    const float scaleBase = 16.0f;

    const float thresholdH = scaleBase * scalePx * scaleDepth * scaleVariance;
    const float thresholdV = thresholdH * 0.5f;

    const uint sampleCount = uint(lerp(4.0f, 16.0f, saturate(c_variance * 100.0f)));

    float sumVariance = c_variance;

    float wSumDiff = 1.0f;
    float wSumSpec = 1.0f;

    float3 s_uvw;

    for (uint i = 0u; i < sampleCount; ++i)
    {
        const float3 s_position = c_position + TBN * float3(rotate2D(PK_POISSON_DISK_POW2_16[i] * thresholdH, rotation), 0.0f);

        if (!Test_WorldToClipUVW(s_position, s_uvw))
        {
            continue;
        }

        const float4 s_nr = SampleWorldNormalRoughness(s_uvw.xy);
        const float s_nd = dot(c_normal, s_nr.xyz);

        if (s_nd < 0.05f)
        {
            continue;
        }

        const float3 s_viewvec = SampleWorldPosition(s_uvw.xy) - s_position;
        const float s_distV = abs(dot(s_viewvec, c_normal));
        const float s_distH = dot(s_viewvec, s_viewvec);

        if (s_distV > thresholdV || s_distH > (thresholdH * thresholdH))
        {
            continue;
        }

        const SH s_diff = GI_Load_SH(s_uvw.xy, PK_GI_DIFF_LVL);
        const SH s_spec = GI_Load_SH(s_uvw.xy, PK_GI_SPEC_LVL);

        float s_lum = GI_SHToLuminance(s_diff, c_normal);

        if (IsNaN(s_lum))
        {
            s_lum = 0.0f;
        }

        const float w_n = pow(s_nd, normalPower);
        const float w_p = 1.0f - (s_distV / thresholdV);
        const float w_d = 1.0f - sqrt(s_distH) / thresholdH;
        const float w_l = exp(-sqrt(abs(c_lum - s_lum)) * wLumMult);
        const float w_r = max(0.0, 1.0 - 10 * abs(c_roughness - s_nr.w)) * wRoughnessMult;

        const float wBase = saturate(w_n * w_p * w_d);
        const float wDiff = wBase *w_l;
        const float wSpec = wBase * w_r;
        
        c_diff = SHAdd(c_diff, s_diff, wDiff);
        c_spec = SHAdd(c_spec, s_spec, wSpec);
        wSumDiff += wDiff;
        wSumSpec += wSpec;

        sumVariance += GI_Load_HistoryVariance(int2(s_uvw.xy * size)).y * pow2(wDiff);
    }

    sumVariance /= pow2(wSumDiff);

    GI_Store_Moments(coord, GI_Load_Moments(coord));
    GI_Store_HistoryVariance(coord, float2(c_history, sumVariance));
    GI_Store_SH(coord, PK_GI_DIFF_LVL, SHScale(c_diff, 1.0f / wSumDiff));
    GI_Store_SH(coord, PK_GI_SPEC_LVL, SHScale(c_spec, 1.0f / wSumSpec));
}
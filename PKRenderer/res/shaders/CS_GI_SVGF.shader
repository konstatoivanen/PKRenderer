#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

#define FILTER_RADIUS 1

const float WAVELET_KERNEL[2][2] =
{
    { 1.0f, 0.5f  },
    { 0.5f, 0.25f }
};

float ComputeVarianceCenter(int2 coord)
{
    const float gaussianKernel[2][2] =
    {
        { 1.0 / 4.0, 1.0 / 8.0  },
        { 1.0 / 8.0, 1.0 / 16.0 }
    };

    float r = 0;

    for (int yy = -FILTER_RADIUS; yy <= FILTER_RADIUS; yy++)
    for (int xx = -FILTER_RADIUS; xx <= FILTER_RADIUS; xx++)
    {
        int2 xy = coord + int2(xx, yy);

        if (!All_InArea(coord, int2(0), pk_ScreenSize.xy))
        {
            continue;
        }

        const float depth = SampleLinearDepth(xy);

        if (!Test_DepthFar(depth))
        {
            continue;
        }
        
        r += GI_Load_HistoryVariance(xy).y * gaussianKernel[abs(xx)][abs(yy)];
    }

    return sqrt(max(r, 0.0));
}

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

    const float centerVariance = ComputeVarianceCenter(coord);
    const float wLumMult = 1.0 / (10.0f * centerVariance + 1e-4f);
    const float wRoughnessMult = saturate(c_roughness * 30);
    const float normalPower = clamp(c_history, 0.25f, 256.0f);

    float sumVariance = c_variance;
    float wSumDiff = 1.0f;
    float wSumSpec = 1.0f;

    for (int xx = -FILTER_RADIUS; xx <= FILTER_RADIUS; ++xx)
    for (int yy = -FILTER_RADIUS; yy <= FILTER_RADIUS; ++yy)
    {
        int2 xy = coord + int2(xx, yy);

        if ((xx == 0 && yy == 0) || !All_InArea(coord, int2(0), pk_ScreenSize.xy))
        {
            continue;
        }

        const float s_depth = SampleLinearDepth(xy);

        if (!Test_DepthFar(s_depth))
        {
            return;
        }

        const SH s_diff = GI_Load_SH(xy, PK_GI_DIFF_LVL);
        const SH s_spec = GI_Load_SH(xy, PK_GI_SPEC_LVL);

        const float4 s_nr = SampleWorldNormalRoughness(xy);
        const float s_nd = saturate(dot(c_normal, s_nr.xyz));
        const float s_lum = GI_SHToLuminance(s_diff, c_normal);

        const float w_z = exp(-abs(c_depth - s_depth) / min(c_depth, s_depth));
        const float w_n = pow(s_nd, normalPower);
        const float w_w = WAVELET_KERNEL[abs(yy)][abs(xx)];
        const float w_l = exp(-sqrt(abs(c_lum - s_lum)) * wLumMult);
        const float w_r = max(0.0, 1.0 - 10 * abs(c_roughness - s_nr.w)) * wRoughnessMult;

        const float wBase = w_z * w_w * w_n;
        const float wDiff = wBase * w_l;
        const float wSpec = wBase * w_r;
   
        c_diff = SHAdd(c_diff, s_diff, wDiff);
        c_spec = SHAdd(c_spec, s_spec, wSpec);
        wSumDiff += wDiff;
        wSumSpec += wSpec;

        sumVariance += GI_Load_HistoryVariance(xy).y * pow2(wDiff);
    }

    sumVariance /= wSumDiff * wSumDiff;

    GI_Store_Moments(coord, GI_Load_Moments(coord));
    GI_Store_HistoryVariance(coord, float2(c_history, sumVariance));
    GI_Store_SH(coord, PK_GI_DIFF_LVL, SHScale(c_diff, 1.0f / wSumDiff));
    GI_Store_SH(coord, PK_GI_SPEC_LVL, SHScale(c_spec, 1.0f / wSumSpec));
}
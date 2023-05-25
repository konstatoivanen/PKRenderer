#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl
#include includes/SharedHistogram.glsl

layout(local_size_x = 16, local_size_y = 2, local_size_z = 1) in;
void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);

    if (Any_GEqual(coord, size))
    {
        return;
    }

    const float depth = SampleLinearDepth(coord);

    if (!Test_DepthFar(depth))
    {
        return;
    }

    const float exposure = GetAutoExposure();
    const float history = GI_Load_HistoryVariance(coord).x;
    const float2 temporalMoments = GI_Load_Moments(coord);
    SH diff = GI_Load_SH(coord, PK_GI_DIFF_LVL);
    SH spec = GI_Load_SH(coord, PK_GI_SPEC_LVL);

    if (history > 4.0f)
    {
        float temporalVariance = max(0.0f, temporalMoments.y - pow2(temporalMoments.x));
        //temporalVariance /= max(1.0f, history - 4.0f);
        //temporalVariance *= exposure;

        GI_Store_Moments(coord, temporalMoments);
        GI_Store_HistoryVariance(coord, float2(history, temporalVariance));
        GI_Store_SH(coord, PK_GI_DIFF_LVL, diff);
        GI_Store_SH(coord, PK_GI_SPEC_LVL, spec);
        return;
    }

    const float3 normal = SampleWorldNormal(coord);
    const float c_l = GI_SHToLuminance(diff, normal);
    
    float wSum = 1.0f;
    float2 moments = float2(c_l, pow2(c_l)) + GI_Load_Moments(coord);
    
    for (int xx = -2; xx <= 2; xx++)
    for (int yy = -2; yy <= 2; yy++)
    {
        const int2 xy = coord + int2(xx, yy);
    
        if ((xx == 0 && yy == 0) || !All_InArea(xy, int2(0), size))
        {
            continue;
        }
    
        const float s_z = SampleLinearDepth(xy);
        
        if (!Test_DepthFar(s_z))
        {
            continue;
        }
    
        const SH s_diff = GI_Load_SH(xy, PK_GI_DIFF_LVL);
        const SH s_spec = GI_Load_SH(xy, PK_GI_SPEC_LVL);
    
        const float3 s_n = SampleWorldNormal(xy);
        const float s_l = GI_SHToLuminance(s_diff, normal);
    
        const float w_z = exp(-abs(depth - s_z) / max(min(depth, s_z), 1e-4f));
        const float w_n = max(0.0f, dot(s_n, normal));
        const float w = w_z * w_n;
    
        diff = SHAdd(diff, s_diff, w);
        spec = SHAdd(spec, s_spec, w);
        moments += (float2(s_l, pow2(s_l)) * GI_Load_Moments(xy)) * w;
        wSum += w;
    }
    
    moments /= wSum;

    float variance = max(0.0f, moments.y - pow2(moments.x));
    variance *= 4.0f / (history + 1.0f);
    variance *= exposure;

    GI_Store_Moments(coord, temporalMoments);
    GI_Store_HistoryVariance(coord, float2(history, variance));
    GI_Store_SH(coord, PK_GI_DIFF_LVL, SHScale(diff, 1.0f / wSum));
    GI_Store_SH(coord, PK_GI_SPEC_LVL, SHScale(spec, 1.0f / wSum));
}
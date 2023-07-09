#version 460
#pragma PROGRAM_COMPUTE
#include includes/GBuffers.glsl
#include includes/SharedSceneGI.glsl
#include includes/SampleDistribution.glsl

// Source: https://developer.download.nvidia.com/video/gputechconf/gtc/2020/presentations/s22699-fast-denoising-with-self-stabilizing-recurrent-blurs.pdf
#define SPEC_ACCUM_BASE_POWER 0.25
#define SPEC_ACCUM_CURVE 2.0

float GetParallaxFactor(float3 motion, float distToSurf, float deltaTime)
{
    return length(motion) / (distToSurf * deltaTime);
}

float GetAntilagSpecular(float roughness, float nv, float parallax)
{
    float acos01sq = saturate(1.0f - nv);
    float a = pow(acos01sq, SPEC_ACCUM_CURVE);
    float b = 1.001f + pow2(roughness);
    float angularSensitivity = (b + a) / (b - a);
    float power = SPEC_ACCUM_BASE_POWER * (1.0f + parallax * angularSensitivity);
    return pow(roughness, power);
}

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 uv = (coord + 0.5f.xx) / size;
    const float depth = SampleViewDepth(coord);

    // Far clip or new backbuffer
    if (pk_FrameIndex.y == 0u || !Test_DepthFar(depth))
    {
        GI_Store_Packed_SampleDiff(coord, uint4(0));
        GI_Store_Packed_SampleSpec(coord, uint2(0));
        return;
    }

    const float4 normalroughness = SampleViewNormalRoughness(coord);
    const float3 normal = normalroughness.xyz;
    const float roughness = normalroughness.w;
    const float zbias = lerp(0.1f, 0.01f, -normal.z);
    const float3 viewpos = SampleViewPosition(coord, size, depth);
    const float viewdist = length(viewpos);
    const float3 viewdir = viewpos / viewdist;

    const float parallax = GetParallaxFactor(pk_ViewSpaceCameraDelta.xyz, viewdist, pk_DeltaTime.x);
    const float antilag_spec = GetAntilagSpecular(roughness, dot(normal, -viewdir), parallax);
    const float antilag_diff = 1.0f;

    const float2 screenuv = ViewToPrevClipUV(viewpos) * size - 0.49f.xx; // Bias to prevent drifting effect.
    const int2 coordprev = int2(screenuv);
    const float2 ddxy = fract(screenuv);

    const float bilinearWeights[2][2] =
    {
        { (1.0 - ddxy.x) * (1.0 - ddxy.y), ddxy.x * (1.0 - ddxy.y) },
        { (1.0 - ddxy.x) * ddxy.y,         ddxy.x * ddxy.y         },
    };
    
    float wSum = 0.0f;
    GISampleDiff c_diff = pk_Zero_GISampleDiff;
    GISampleSpec c_spec = pk_Zero_GISampleSpec;

    for (int yy = 0; yy <= 1; ++yy)
    for (int xx = 0; xx <= 1; ++xx)
    {
        const int2 xy = coordprev + int2(xx, yy);
        const float s_depth = SamplePreviousViewDepth(xy);
        const float3 s_normal = SamplePreviousViewNormal(xy);
        const GISampleDiff s_diff = GI_Load_SampleDiff(xy);
        const GISampleSpec s_spec = GI_Load_SampleSpec(xy);

        const float w_b = bilinearWeights[yy][xx];
        const float w_n = dot(normal, s_normal);
        const float w = w_b * w_n;

        if (Test_InScreen(xy) &&
            Test_DepthFar(s_depth) &&
            Test_DepthReproject(depth, s_depth, zbias) &&
            w_b > 1e-4f &&
            w_n > 0.05f)
        {
            c_diff.sh = SH_Add(c_diff.sh, s_diff.sh, w);
            c_diff.ao += s_diff.ao * w;
            c_spec.radiance += s_spec.radiance * w;
            c_spec.ao += s_spec.ao * w;
            c_diff.history += s_diff.history * w;
            c_spec.history += s_spec.history * w;
            wSum += w;
        }
     
    }

    // Try to find valid samples with a bilateral cross filter
    if (wSum <= 1e-4f)
    {
        wSum = 0.0f;

        for (int yy = -1; yy <= 1; yy++)
        for (int xx = -1; xx <= 1; xx++)
        {
            const int2 xy = coordprev + int2(xx, yy);
            const float s_depth = SamplePreviousViewDepth(xy);
            const float3 s_normal = SamplePreviousViewNormal(xy);
            const GISampleDiff s_diff = GI_Load_SampleDiff(xy);
            const GISampleSpec s_spec = GI_Load_SampleSpec(xy);

            const float w_z = 1.0f / (1e-4f + abs(depth - s_depth));
            const float w_n = dot(normal, s_normal);
            const float w = w_z * w_n;

            if (Test_InScreen(xy) &&
                Test_DepthFar(s_depth) &&
                Test_DepthReproject(depth, s_depth, zbias) &&
                w_n > 0.05f)
            {
                wSum += w;
                c_diff.sh = SH_Add(c_diff.sh, s_diff.sh, w);
                c_diff.ao += s_diff.ao * w;
                c_spec.radiance += s_spec.radiance * w;
                c_spec.ao += s_spec.ao * w;
                c_diff.history += s_diff.history * w;
                c_spec.history += s_spec.history * w;
            }
        }
    }

    // Normalize weights
    if (wSum > 1e-4f)
    {
        c_diff.sh = SH_Scale(c_diff.sh, 1.0f / wSum);
        c_diff.ao /= wSum;
        c_spec.radiance /= wSum;
        c_spec.ao /= wSum;
        c_diff.history = clamp((c_diff.history / wSum) + 1.0f, 1.0f, PK_GI_MAX_HISTORY * antilag_diff);
        c_spec.history = clamp((c_spec.history / wSum) + 1.0f, 1.0f, PK_GI_MAX_HISTORY * antilag_spec);
    }

    const bool invalidDiff = Any_IsNaN(c_diff.sh.Y) || Any_IsNaN(c_diff.sh.CoCg) || isnan(c_diff.ao) || isnan(c_diff.history);
    const bool invalidSpec = Any_IsNaN(c_spec.radiance) || isnan(c_spec.ao) || isnan(c_spec.history);

    uint4 packedDiff = invalidDiff ? uint4(0) : GI_Pack_SampleDiff(c_diff);
    uint2 packedSpec = invalidSpec ? uint2(0) : GI_Pack_SampleSpec(c_spec);

    GI_Store_Packed_SampleDiff(coord, packedDiff);
    GI_Store_Packed_SampleSpec(coord, packedSpec);
}
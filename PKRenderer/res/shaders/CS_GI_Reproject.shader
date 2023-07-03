#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

// Source: https://developer.download.nvidia.com/video/gputechconf/gtc/2020/presentations/s22699-fast-denoising-with-self-stabilizing-recurrent-blurs.pdf
#define SPEC_ACCUM_BASE_POWER 0.25
#define SPEC_ACCUM_CURVE 2.0

float GetParallaxFactor(float3 motion, float distToSurf, float deltaTime)
{
    return length(motion) / (distToSurf * deltaTime);
}

float GetMaxSpecularHistory(float roughness, float NoV, float parallax)
{
    float acos01sq = saturate(1.0 - NoV);
    float a = pow(acos01sq, SPEC_ACCUM_CURVE);
    float b = 1.001 + pow2(roughness);
    float angularSensitivity = (b + a) / (b - a);
    float power = SPEC_ACCUM_BASE_POWER * (1.0 + parallax * angularSensitivity);
    return max(1.0f, PK_GI_MAX_HISTORY * pow(roughness, power));
}

layout(local_size_x = PK_W_ALIGNMENT_8, local_size_y = PK_W_ALIGNMENT_8, local_size_z = 1) in;
void main()
{
    const int2 size = int2(pk_ScreenSize.xy);
    const int2 coord = int2(gl_GlobalInvocationID.xy);
    const float2 uv = (coord + 0.5f.xx) / size;
    const float depth = SampleLinearDepth(coord);

    // Far clip or new backbuffer
    if (pk_FrameIndex.y == 0u || !Test_DepthFar(depth))
    {
        GI_Store_Packed_SampleDiff(coord, uint4(0));
        GI_Store_Packed_SampleSpec(coord, uint2(0));
        return;
    }
   
    GISampleDiff c_diff = pk_Zero_GISampleDiff;
    GISampleSpec c_spec = pk_Zero_GISampleSpec;

    const float4 normalRoughness = SampleViewNormalRoughness(coord);
    const float3 normal = normalRoughness.xyz;
    const float depthBias = lerp(0.1f, 0.01f, -normal.z);
    const float4 viewpos = float4(SampleViewPosition(coord, size, depth), 1.0f);

    const float parallax = GetParallaxFactor(pk_ViewSpaceCameraDelta.xyz, length(viewpos), pk_DeltaTime.x);
    const float NoV = dot(normal, -normalize(viewpos.xyz));
    const float maxSpecHistory = GetMaxSpecularHistory(normalRoughness.w, NoV, parallax);

    const float2 uvPrev = ClipToUVW(mul(pk_MATRIX_LD_P, viewpos)).xy * size - 0.49f.xx; // Bias to prevent drifting effect.
    const int2 coordPrev = int2(uvPrev);
    const float2 ddxy = uvPrev - coordPrev;

    const float bilinearWeights[2][2] =
    {
        { (1.0 - ddxy.x) * (1.0 - ddxy.y), ddxy.x * (1.0 - ddxy.y) },
        { (1.0 - ddxy.x) * ddxy.y,         ddxy.x * ddxy.y         },
    };
    
    float wSum = 0.0f;

    for (int yy = 0; yy <= 1; ++yy)
    for (int xx = 0; xx <= 1; ++xx)
    {
        const int2 xy = coordPrev + int2(xx, yy);
        float weight = bilinearWeights[yy][xx];

        const float depthPrev = SamplePreviousLinearDepth(xy);
        const float3 normalPrev = SamplePreviousViewNormal(xy);
        const GISampleDiff s_diff = GI_Load_SampleDiff(xy);
        const GISampleSpec s_spec = GI_Load_SampleSpec(xy);

        const float normalDot = dot(normalPrev, normal);

        if (weight > 1e-4f &&
            All_InArea(xy, int2(0), size) &&
            Test_DepthFar(depthPrev) && 
            Test_DepthReproject(depth, depthPrev, depthBias) && 
            normalDot > 0.05f)
        {
            weight *= normalDot;
            wSum += weight;

            c_diff.sh = SH_Add(c_diff.sh, s_diff.sh, weight);
            c_diff.ao += s_diff.ao * weight;
            c_spec.radiance += s_spec.radiance * weight;
            c_spec.ao += s_spec.ao * weight;
            c_diff.history += s_diff.history * weight;
            c_spec.history += s_spec.history * weight;
        }
     
    }

    // Try to find valid samples with a bilateral cross filter
    if (wSum <= 1e-4f)
    {
        wSum = 0.0f;

        for (int yy = -1; yy <= 1; yy++)
        for (int xx = -1; xx <= 1; xx++)
        {
            const int2 xy = coordPrev + int2(xx, yy);

            const float depthPrev = SamplePreviousLinearDepth(xy);
            const float3 normalPrev = SamplePreviousViewNormal(xy);
            const GISampleDiff s_diff = GI_Load_SampleDiff(xy);
            const GISampleSpec s_spec = GI_Load_SampleSpec(xy);

            const float normalDot = dot(normalPrev, normal);
            const float weight = normalDot / (1e-4f + abs(depth - depthPrev));

            if (All_InArea(xy, int2(0), size) &&
                Test_DepthFar(depthPrev) &&
                Test_DepthReproject(depth, depthPrev, depthBias) && 
                normalDot > 0.05f)
            {
                wSum += weight;
                c_diff.sh = SH_Add(c_diff.sh, s_diff.sh, weight);
                c_diff.ao += s_diff.ao * weight;
                c_spec.radiance += s_spec.radiance * weight;
                c_spec.ao += s_spec.ao * weight;
                c_diff.history += s_diff.history * weight;
                c_spec.history += s_spec.history * weight;
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
        c_diff.history = min(PK_GI_MAX_HISTORY, (c_diff.history / wSum) + 1.0f);
        c_spec.history = min(maxSpecHistory, (c_spec.history / wSum) + 1.0f);
    }

    const bool invalidDiff = Any_IsNaN(c_diff.sh.Y) || Any_IsNaN(c_diff.sh.CoCg) || isnan(c_diff.ao) || isnan(c_diff.history);
    const bool invalidSpec = Any_IsNaN(c_spec.radiance) || isnan(c_spec.ao) || isnan(c_spec.history);

    uint4 packedDiff = invalidDiff ? uint4(0) : GI_Pack_SampleDiff(c_diff);
    uint2 packedSpec = invalidSpec ? uint2(0) : GI_Pack_SampleSpec(c_spec);

    GI_Store_Packed_SampleDiff(coord, packedDiff);
    GI_Store_Packed_SampleSpec(coord, packedSpec);
}
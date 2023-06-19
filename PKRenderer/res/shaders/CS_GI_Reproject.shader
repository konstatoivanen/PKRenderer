#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

void AddWeightedSample(const int2 coord, inout GISampleFull o, const float weight)
{
    GISampleFull s = GI_Load_SampleFull(coord);
    o.diff.sh = SHAdd(o.diff.sh, s.diff.sh, weight);
    o.diff.ao += s.diff.ao * weight;
    o.spec.radiance += s.spec.radiance * weight;
    o.spec.ao += s.spec.ao * weight;
    o.meta.historyDiff += s.meta.historyDiff * weight;
    o.meta.historySpec += s.meta.historySpec * weight;
    o.meta.moments += s.meta.moments * weight;
}

// Source: https://developer.download.nvidia.com/video/gputechconf/gtc/2020/presentations/s22699-fast-denoising-with-self-stabilizing-recurrent-blurs.pdf
#define SPEC_ACCUM_BASE_POWER 0.25
#define SPEC_ACCUM_CURVE 32.0

float GetParallaxFactor(float3 motion, float distToSurf, float deltaTime)
{
    return length(motion) / (distToSurf * deltaTime);
}

float GetMaxSpecularHistory(float roughness, float NoV, float parallax)
{
    float acos01sq = saturate(1.0 - NoV);
    float a = pow(acos01sq, SPEC_ACCUM_CURVE);
    float b = 1.001 + roughness * roughness;
    float angularSensitivity = (b + a) / (b - a);
    float power = SPEC_ACCUM_BASE_POWER * (1.0 + parallax * angularSensitivity);
    return PK_GI_MAX_HISTORY * pow(roughness, power);
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
        GI_Store_Packed_SampleFull(coord, uint4(0), uint2(0), uint2(0));
        return;
    }
   
    GISampleFull filtered = pk_Zero_GISampleFull;

    const float4 normalRoughness = SampleViewNormalRoughness(coord);
    const float3 normal = normalRoughness.xyz;
    const float depthBias = lerp(0.1f, 0.01f, -normal.z);
    const float4 viewpos = float4(SampleViewPosition(coord, size, depth), 1.0f);

    const float parallax = GetParallaxFactor(pk_ViewSpaceCameraDelta.xyz, length(viewpos), pk_DeltaTime.x);
    const float NoV = dot(normal, -normalize(viewpos.xyz));
    const float maxSpecHistory = PK_GI_MAX_HISTORY;// GetMaxSpecularHistory(normalRoughness.w, NoV, parallax);

    const float2 uvPrev = ClipToUVW(mul(pk_MATRIX_LD_P, viewpos)).xy * size - 0.5.xx;
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
        
        if (!All_InArea(xy, int2(0), size) || weight < 1e-4f)
        {
            continue;
        }

        const float depthPrev = SamplePreviousLinearDepth(xy);

        if (!Test_DepthFar(depthPrev))
        {
            continue;
        }

        const float3 normalPrev = SamplePreviousViewNormal(xy);
        const float normalDot = dot(normalPrev, normal);

        if (!Test_DepthReproject(depth, depthPrev, depthBias) || normalDot <= 0.05f)
        {
            continue;
        }
     
        weight *= normalDot;
        wSum += weight;
        AddWeightedSample(xy, filtered, weight);
    }

    // Try to find valid samples with a bilateral cross filter
    if (wSum <= 1e-4f)
    {
        wSum = 0.0f;

        for (int yy = -1; yy <= 1; yy++)
        for (int xx = -1; xx <= 1; xx++)
        {
            const int2 xy = coordPrev + int2(xx, yy);

            if (!All_InArea(xy, int2(0), size))
            {
                continue;
            }

            const float depthPrev = SamplePreviousLinearDepth(xy);

            if (!Test_DepthFar(depthPrev))
            {
                continue;
            }

            const float3 normalPrev = SamplePreviousViewNormal(xy);
            const float normalDot = dot(normalPrev, normal);

            if (Test_DepthReproject(depth, depthPrev, depthBias) && normalDot > 0.05f)
            {
                wSum += 1.0f;
                AddWeightedSample(xy, filtered, 1.0f);
            }
        }
    }

    // Normalize weights
    if (wSum > 1e-4f)
    {
        filtered.diff.sh = SHScale(filtered.diff.sh, 1.0f / wSum);
        filtered.diff.ao /= wSum;
        filtered.spec.radiance /= wSum;
        filtered.spec.ao /= wSum;
        filtered.meta.historyDiff = min(PK_GI_MAX_HISTORY, (filtered.meta.historyDiff / wSum) + 1.0f);
        filtered.meta.historySpec = min(maxSpecHistory, (filtered.meta.historySpec / wSum) + 1.0f);
        filtered.meta.moments /= wSum;
    }

    const bool invalidDiff = Any_IsNaN(filtered.diff.sh.Y) || Any_IsNaN(filtered.diff.sh.CoCg) || isnan(filtered.diff.ao);
    const bool invalidSpec = Any_IsNaN(filtered.spec.radiance) || isnan(filtered.spec.ao);
    const bool invalidMeta = isnan(filtered.meta.historyDiff) || isnan(filtered.meta.historySpec) || Any_IsNaN(filtered.meta.moments);

    uint4 packedDiff = invalidDiff ? uint4(0) : GI_Pack_SampleDiff(filtered.diff);
    uint2 packedSpec = invalidSpec ? uint2(0) : GI_Pack_SampleSpec(filtered.spec);
    uint2 packedMeta = invalidMeta ? uint2(0) : GI_Pack_SampleMeta(filtered.meta);

    GI_Store_Packed_SampleFull(coord, packedDiff, packedSpec, packedMeta);
}
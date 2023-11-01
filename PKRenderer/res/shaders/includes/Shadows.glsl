#pragma once
#include Common.glsl
#include Kernels.glsl

PK_DECLARE_SET_PASS uniform sampler2DArray pk_ShadowmapAtlas;
PK_DECLARE_SET_PASS uniform sampler2D pk_ShadowmapScreenSpace;

#define SHADOW_NEAR_BIAS 0.06f
#define SHADOW_HARD_EDGE_FADE_FACTOR 20.0hf
#define SHADOW_PCSS_SUBGROUP 1
#define SHADOW_SIZE textureSize(pk_ShadowmapAtlas, 0)

float Shadow_GradientNoise(float2 coord, uint frame)
{
    // "Interleaved gradient noise", by Jorge Jimenez,
    // http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
    frame = frame & 63u; // need to periodically reset frame to avoid numerical issues
    float x = coord.x + 5.588238f * float(frame);
    float y = coord.y + 5.588238f * float(frame);
    return fract(52.9829189f * fract(0.06711056f * x + 0.00583715f * y));
}

//Source: http://the-witness.net/news/2013/09/shadow-mapping-summary-part-1/
float3 Shadow_GetSamplingOffset(const float3 N, const float3 L) 
{
    float cosa = saturate(dot(N, L));
    float scaleN = sqrt(1.0f - pow2(cosa));
    float scaleL = scaleN / (1e-4f + cosa);
    return (N * scaleN + L * min(2, scaleL)) * SHADOW_NEAR_BIAS;
}

half2 Shadow_GatherMax(const uint index, const float2 uv, const float z)
{
    const float4 depths = textureGather(pk_ShadowmapAtlas, float3(uv, index), 0);
    const half deltaDepth = half(z - max(max(max(depths.x, depths.y), depths.z), depths.w));
    const half shadowFade = -deltaDepth * SHADOW_HARD_EDGE_FADE_FACTOR + 1.0hf;
    return half2(deltaDepth, 1.0hf) * step(shadowFade, 0.0hf);
}

half ShadowTest_PCF2x2(const uint index, const float2 uv, const float z) 
{ 
    const float2 f = fract(uv * SHADOW_SIZE.xx - 0.5f.xx).xy;
    const float4 fw = float4(f.xy, 1.0hf - f.xy);
    half4 s = half4(textureGather(pk_ShadowmapAtlas, float3(uv, index), 0).wzxy - z.xxxx);
    s = clamp(s * SHADOW_HARD_EDGE_FADE_FACTOR + 1.0hf, 0.0hf, 1.0hf);
    return dot(s, half4(fw.zxzx * fw.wwyy));
}

half ShadowTest_PCF3x3Gaussian(const uint index, const float2 uv, const float z)
{
    const float2 coord = (uv * SHADOW_SIZE.xx) + 0.5f.xx;
    const float2 base = (floor(coord) - 0.5f.xx) / SHADOW_SIZE.xx;
    const float2 st = fract(coord);

    const float2 uw = float2(3 - 2 * st.x, 1 + 2 * st.x);
    float2 u = float2((2 - st.x) / uw.x - 1, (st.x) / uw.y + 1);
    u /= SHADOW_SIZE.x;

    const float2 vw = float2(3 - 2 * st.y, 1 + 2 * st.y);
    float2 v = float2((2 - st.y) / vw.x - 1, (st.y) / vw.y + 1);
    v /= SHADOW_SIZE.x;

    half shadow = 0.0hf;
    shadow += half(uw[0] * vw[0]) * ShadowTest_PCF2x2(index, base + float2(u[0], v[0]), z);
    shadow += half(uw[1] * vw[0]) * ShadowTest_PCF2x2(index, base + float2(u[1], v[0]), z);
    shadow += half(uw[0] * vw[1]) * ShadowTest_PCF2x2(index, base + float2(u[0], v[1]), z);
    shadow += half(uw[1] * vw[1]) * ShadowTest_PCF2x2(index, base + float2(u[1], v[1]), z);
    return shadow / 16.0hf;
}

half ShadowTest_Dither16(const uint index, const float2 uv, const float z)
{
    const half ditherAngle = half(Shadow_GradientNoise(PK_GET_PROG_COORD, pk_FrameRandom.y) * PK_TWO_PI);
    const half scale = 2.5hf / half(SHADOW_SIZE.x);
    const half sina = sin(ditherAngle) * scale;
    const half cosa = cos(ditherAngle) * scale;
    const half2x2 basis = half2x2(sina, cosa, -cosa, sina);

    half shadow = 0.0hf;
    
    //[[unroll]]
    for (uint i = 0u; i < 16u; ++i)
    {
        shadow += ShadowTest_PCF2x2(index, uv + basis * half2(PK_SPIRAL_DISK_16[i]), z);
    }

    return shadow / 16.0hf;
}

half ShadowTest_PCSS(const uint index, float2 uv, const float z, float radius)
{
    const half ditherAngle = half(Shadow_GradientNoise(PK_GET_PROG_COORD, pk_FrameRandom.y) * PK_TWO_PI);
    const half maxOffset = 16.0hf / half(SHADOW_SIZE.x);
    const half sina = sin(ditherAngle) * maxOffset;
    const half cosa = cos(ditherAngle) * maxOffset;
    const half2x2 basis = half2x2(sina, cosa, -cosa, sina);

    half2 avgZ = 0.0hf.xx;

    for (uint i = 0u; i < 16u; ++i)
    {
        const half2 offset = basis * half2(PK_POISSON_DISK_16_POW[i].xy);
        avgZ += Shadow_GatherMax(index, uv + offset, z);
    }

    const half minOffset = 1.5hf / half(SHADOW_SIZE.x);

#if defined(SHADER_STAGE_FRAGMENT) && SHADOW_PCSS_SUBGROUP == 1
    float2 maxDerivative = max(abs(dFdx(uv)), abs(dFdy(uv)));
    float quadWeight = 0.8f * max(1.0f - max(maxDerivative.x, maxDerivative.y) / float(minOffset), 0.0f);
    const float2 gradients = floor(2.0f * fract(gl_FragCoord.xy * 0.5f));
    
    float depthAvg = avgZ.x;
    depthAvg = depthAvg + (0.5f - gradients.x) * dFdxFine(depthAvg);
    depthAvg = depthAvg + (0.5f - gradients.y) * dFdyFine(depthAvg);
    depthAvg = lerp(float(avgZ.x), 4.0f * depthAvg, quadWeight);

    float sumAvg = avgZ.y;
    sumAvg = sumAvg + (0.5f - gradients.x) * dFdxFine(sumAvg);
    sumAvg = sumAvg + (0.5f - gradients.y) * dFdyFine(sumAvg);
    sumAvg = lerp(float(avgZ.y), 4.0f * sumAvg, quadWeight);

    avgZ.x = half(depthAvg);
    avgZ.y = half(sumAvg);
#endif

    [[branch]]
    if (avgZ.y == 0.0hf)
    {
        return 1.0hf;
    }

    [[branch]]
    if (avgZ.y > 15.5hf)
    {
        return 0.0hf;
    }

    avgZ.x /= avgZ.y;
    avgZ.x = clamp(half(radius) * avgZ.x, minOffset / maxOffset, 1.0hf);

    half shadow = 0.0hf;
    
    for (uint i = 0u; i < 16u; ++i)
    {
        const half2 offset = basis * half2(PK_POISSON_DISK_16_POW[i].xy) * avgZ.x;
        shadow += ShadowTest_PCF2x2(index, uv + offset, z);
    }

    shadow /= 16.0hf;

#if defined(SHADER_STAGE_FRAGMENT) && SHADOW_PCSS_SUBGROUP == 1
    float shadowAvg = shadow;
    shadowAvg = shadowAvg + (0.5f - gradients.x) * dFdxFine(shadowAvg);
    shadowAvg = shadowAvg + (0.5f - gradients.y) * dFdyFine(shadowAvg);
    shadow = half(lerp(float(shadow), shadowAvg, quadWeight * 0.5f));
#endif

    return shadow;
}

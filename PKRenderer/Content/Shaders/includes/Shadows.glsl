#pragma once
#include "Common.glsl"
#include "Kernels.glsl"

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
float2 Shadow_GetBiasFactors(const float3 N, const float3 L)
{
    const float cosa = saturate(dot(N, L));
    const float scaleN = sqrt(1.0f - pow2(cosa));
    const float scaleL = scaleN / (1e-4f + cosa);
    return float2(scaleN, min(2.0f, scaleL));
}

half2 Shadow_GatherMax(const uint index, const float2 uv, const float z)
{
    const float4 depths = textureGather(pk_ShadowmapAtlas, float3(uv, index), 0);
    const half deltaDepth = half(z - max(max(max(depths.x, depths.y), depths.z), depths.w));
    const half shadowFade = -deltaDepth * SHADOW_HARD_EDGE_FADE_FACTOR + 1.0hf;
    return half2(deltaDepth, 1.0f) * step(shadowFade, 0.0hf);
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

half ShadowTest_Volumetrics4(const uint index, const float4 uvrange, const float z)
{
    half shadow = 0.0hf;

    // Volumetrics pcf by dithering along view axis. More stable than random offsets on the shadow plane.
    for (uint i = 0u; i < 4u; ++i)
    {
        const float dither = Shadow_GradientNoise(PK_GET_PROG_COORD, pk_FrameIndex.y + i);
        const float2 uv = lerp(uvrange.xy, uvrange.zw, dither);
        shadow += ShadowTest_PCF2x2(index, uv, z);
    }

    return shadow * 0.25hf;
}

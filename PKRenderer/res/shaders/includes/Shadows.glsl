#pragma once
#include Common.glsl

PK_DECLARE_SET_PASS uniform sampler2DArray pk_ShadowmapAtlas;
PK_DECLARE_SET_PASS uniform sampler2D pk_ShadowmapScreenSpace;

#define SHADOW_NEAR_BIAS 0.06f
#define SHADOW_HARD_EDGE_FADE_FACTOR 20.0hf
#define SHADOW_PCSS_SUBGROUP 1
#define SHADOW_SIZE textureSize(pk_ShadowmapAtlas, 0)

#define SHADOW_DECLARE_SPIRAL_OFFFSETS(name) \
const half2 name[16] =                       \
{                                            \
    half2( 0.1250hf,  0.0000hf),             \
    half2(-0.1250hf,  0.0000hf),             \
    half2(-0.1768hf, -0.1768hf),             \
    half2( 0.1768hf,  0.1768hf),             \
    half2(-0.0000hf,  0.3750hf),             \
    half2( 0.0000hf, -0.3750hf),             \
    half2( 0.3536hf, -0.3536hf),             \
    half2(-0.3536hf,  0.3536hf),             \
    half2(-0.6250hf, -0.0000hf),             \
    half2( 0.6250hf,  0.0000hf),             \
    half2( 0.5303hf,  0.5303hf),             \
    half2(-0.5303hf, -0.5303hf),             \
    half2(-0.0000hf, -0.8750hf),             \
    half2( 0.0000hf,  0.8750hf),             \
    half2(-0.7071hf,  0.7071hf),             \
    half2( 0.7071hf, -0.7071hf),             \
};                                           \

float Shadow_GradientNoise()
{
    // "Interleaved gradient noise", by Jorge Jimenez,
    // http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
    const float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    const float frameRandom = make_unorm(pk_FrameRandom.x);
    float n = fract(magic.z * fract(dot(float2(PK_GET_PROG_COORD + frameRandom.xx), magic.xy))); 
    return n * PK_TWO_PI;
}

//Source: http://the-witness.net/news/2013/09/shadow-mapping-summary-part-1/
float3 Shadow_GetSamplingOffset(const float3 N, const float3 L) 
{
    float cosa = saturate(dot(N, L));
    float scaleN = sqrt(1.0f - pow2(cosa));
    float scaleL = scaleN / (1e-4f + cosa);
    return (N * scaleN + L * min(2, scaleL)) * SHADOW_NEAR_BIAS;
}

half ShadowTest_PCF2x2(const uint index, const float2 uv, const float z) 
{ 
    const float2 f = fract(uv * SHADOW_SIZE.xx - 0.5f.xx).xy;
    const float4 fw = float4(f.xy, 1.0hf - f.xy);
    half4 s = half4(textureGather(pk_ShadowmapAtlas, float3(uv, index), 0).wzxy - z.xxxx);
    s = clamp(s * SHADOW_HARD_EDGE_FADE_FACTOR + 1.0hf, 0.0hf, 1.0hf);
    return dot(s, half4(fw.zxzx * fw.wwyy));
}

half ShadowTest_Spiral16(const uint index, const float2 uv, const float z)
{
    SHADOW_DECLARE_SPIRAL_OFFFSETS(offsets)

    const half dither = half(Shadow_GradientNoise());
    const half2 rotation = make_rotation(dither) * 2.5hf / half(SHADOW_SIZE.x);
    
    half shadow = 0.0hf;
    
    [[unroll]]
    for (uint i = 0u; i < 16u; ++i)
    {
        shadow += ShadowTest_PCF2x2(index, uv + rotate2D(offsets[i], rotation), z);
    }

    return shadow / 16.0hf;
}

float DFDAverage(float2 s, float v)
{
#if defined(SHADER_STAGE_FRAGMENT) 
    v = v + (0.5f - s.x) * dFdxFine(v);
    return v + (0.5f - s.y) * dFdyFine(v);
#else
    return v;
#endif
}

float2 DFDAverage(float2 s, float2 v)
{
#if defined(SHADER_STAGE_FRAGMENT) 
    v = v + (0.5f - s.x) * dFdxFine(v);
    return v + (0.5f - s.y) * dFdyFine(v);
#else
    return v;
#endif
}

half ShadowTest_PCSS(const uint index, float2 uv, const float z, float radius)
{
    SHADOW_DECLARE_SPIRAL_OFFFSETS(offsets);

    const half dither = half(Shadow_GradientNoise());
    half2 rotation = make_rotation(dither);
    
    const half maxOffset = 16.0hf / half(SHADOW_SIZE.x);
    half2 avg_z = 0.0hf.xx;

    for (uint i = 0u; i < 16u; ++i)
    {
        const half2 offset = rotate2D(offsets[i], rotation) * maxOffset;
        const float4 depths = textureGather(pk_ShadowmapAtlas, float3(uv + offset, index), 0);
        const float s_z = cmax(depths);
        const float s_d = (s_z - z) * float(SHADOW_HARD_EDGE_FADE_FACTOR) + 1.0f;
        avg_z += half2(half(s_z), 1.0hf) * half(step(s_d, 0.0f));
    }

    const half minOffset = 1.5hf / half(SHADOW_SIZE.x);

#if defined(SHADER_STAGE_FRAGMENT) && SHADOW_PCSS_SUBGROUP == 1
    float2 maxDerivative = max(abs(dFdx(uv)), abs(dFdy(uv)));
    float quadWeight = 0.8f * max(1.0f - max(maxDerivative.x, maxDerivative.y) / float(minOffset), 0.0f);

    const float2 s = floor(2.0f * fract(gl_FragCoord.xy * 0.5f));
    
    float ds = lerp(float(avg_z.x), 4.0f * DFDAverage(s, avg_z.x), quadWeight);
    float sw = lerp(float(avg_z.y), 4.0f * DFDAverage(s, avg_z.y), quadWeight);

    avg_z.y = half(sw);
    avg_z.x = half(ds);
#endif

    [[branch]]
    if (avg_z.y == 0.0hf)
    {
        return 1.0hf;
    }

    [[branch]]
    if (avg_z.y > 15.5hf)
    {
        return 0.0hf;
    }

    avg_z.x /= avg_z.y;
    avg_z.x = clamp(half(radius) * half(z - avg_z.x), 0.0hf, 1.0hf);
    rotation *= lerp(minOffset, maxOffset, avg_z.x);

    half shadow = 0.0hf;
    
    for (uint i = 0u; i < 16u; ++i)
    {
        const half2 offset = rotate2D(offsets[i], rotation);
        shadow += ShadowTest_PCF2x2(index, uv + offset, z);
    }

    shadow /= 16.0hf;

#if defined(SHADER_STAGE_FRAGMENT) && SHADOW_PCSS_SUBGROUP == 1
    shadow = half(lerp(float(shadow), DFDAverage(s, float(shadow)), quadWeight * 0.5f));
#endif

    return shadow;
}

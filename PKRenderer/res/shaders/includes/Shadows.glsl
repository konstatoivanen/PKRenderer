#pragma once
#include Common.glsl

PK_DECLARE_SET_PASS uniform sampler2DArray pk_ShadowmapAtlas;

#define SHADOW_NEAR_BIAS 0.06f
#define SHADOW_HARD_EDGE_FADE_FACTOR 20.0f
#define SHADOW_SIZE textureSize(pk_ShadowmapAtlas, 0)

float Shadow_GradientNoise()
{
#if defined(SHADER_STAGE_FRAGMENT) 
    // "Interleaved gradient noise", by Jorge Jimenez,
    // http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
    const float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    float n = -1.0f + 2.0f * fract(magic.z * fract(dot(gl_FragCoord.xy, magic.xy))); 
    n += make_unorm(pk_FrameRandom.x);
    n *= PK_PI;
    return n;
#else
    return 0.0f;
#endif
}

//Source: http://the-witness.net/news/2013/09/shadow-mapping-summary-part-1/
float3 Shadow_GetSamplingOffset(const float3 N, const float3 L) 
{
    float cosa = saturate(dot(N, L));
    float scaleN = sqrt(1.0f - pow2(cosa));
    float scaleL = scaleN / (1e-4f + cosa);
    return (N * scaleN + L * min(2, scaleL)) * SHADOW_NEAR_BIAS;
}

float ShadowTest_PCF2x2(const uint index, const float2 uv, const float z) 
{ 
    const float4 f = fma(fract(uv * SHADOW_SIZE.xx - 0.5f.xx).xyxy, float4(1.0f.xx, -1.0f.xx), float4(0.0f.xx, 1.0f.xx));
    const float4 s = textureGather(pk_ShadowmapAtlas, float3(uv, index), 0);
    return dot(saturate((s - z.xxxx) * SHADOW_HARD_EDGE_FADE_FACTOR + 1.0f), f.zxzx * f.wwyy);
}

float ShadowTest_Poisson16(const uint index, const float2 uv, const float z)
{
    float2 offsets [16]	=	
    {
        float2( 0.1250f,  0.0000f),
        float2(-0.1250f,  0.0000f),
        float2(-0.1768f, -0.1768f),
        float2( 0.1768f,  0.1768f),
        float2(-0.0000f,  0.3750f),
        float2( 0.0000f, -0.3750f),
        float2( 0.3536f, -0.3536f),
        float2(-0.3536f,  0.3536f),
        float2(-0.6250f, -0.0000f),
        float2( 0.6250f,  0.0000f),
        float2( 0.5303f,  0.5303f),
        float2(-0.5303f, -0.5303f),
        float2(-0.0000f, -0.8750f),
        float2( 0.0000f,  0.8750f),
        float2(-0.7071f,  0.7071f),
        float2( 0.7071f, -0.7071f),
    };

    const float2 rotation = make_rotation(Shadow_GradientNoise()) * 2.5f / SHADOW_SIZE.x;
    
    float shadow = 0.0f;
    
    for (uint i = 0u; i < 16u; ++i)
    {
        const float2 offset = rotate2D(offsets[i], rotation);
        shadow += ShadowTest_PCF2x2(index, uv.xy + offset, z);
    }

    return shadow / 16.0f;
}
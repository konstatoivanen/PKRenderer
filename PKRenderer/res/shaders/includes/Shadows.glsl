#pragma once
#include Common.glsl

PK_DECLARE_SET_PASS uniform sampler2DArray pk_ShadowmapAtlas;

#define SHADOW_USE_LBR  1
#define SHADOW_USE_DERIVATIVE_BIAS 1
#define SHADOW_LBR 0.2f

#if SHADOW_USE_LBR == 1
    float LBR(float shadow) { return smoothstep(SHADOW_LBR, 1.0f, shadow); }
#else
    #define LBR(shadow) (shadow)
#endif

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

float3 Shadow_GetReceiverBias(const float3 uvdist, const uint index)
{
#if defined(SHADER_STAGE_FRAGMENT) && SHADOW_USE_DERIVATIVE_BIAS == 1 
    const float3 dx = dFdx(uvdist);
    const float3 dy = dFdy(uvdist);

    float3 bias = 0.0f.xxx;
    bias.x = dy.y * dx.z - dx.y * dy.z;
    bias.y = dx.x * dy.z - dy.x * dx.z;
    bias.xy *= 1.0f / ((dx.x * dy.y) - (dx.y * dy.x));
    bias.z = -min(dot(textureSize(pk_ShadowmapAtlas, 0).xx, abs(bias.xy)), 0.01f);

    // Cascade delta, derivatives are unreliable.
    return fwidth(float(index)) > 0.0f ? float3(0,0,-0.1f) : bias;
 #else
    return float3(0,0,-0.01f);
 #endif
}

float3 Shadow_CombineCoords(const float3 uvz, const float2 offset, const float3 bias)
{
    return float3(uvz.xy + offset, uvz.z + bias.z + dot(offset, bias.xy));
}

// Not in use & support has been removed. 
float ShadowTest_Variance(const uint index, const float3 uvz)
{
    const float2 moments = tex2D(pk_ShadowmapAtlas, float3(uvz.xy, index)).xy;
    const float variance = moments.y - moments.x * moments.x;
    const float difference = uvz.z - moments.x;
    return min(LBR(variance / (variance + pow2(difference))) + step(difference, 0.1f), 1.0f);
}

float ShadowTest_Single(const uint index, const float3 uvz) { return  float(tex2D(pk_ShadowmapAtlas, float3(uvz.xy, index)).x > uvz.z); }

float ShadowTest_Dither8(const uint index, const float3 uvz)
{
    float2 offsets [16]	=	
    {
        +float2( +0.1250, +0.0000),	// 0,015625
        -float2( +0.1250, +0.0000),	// 0,015625
        +float2( -0.1768, -0.1768),	// 0,06251648
        -float2( -0.1768, -0.1768),	// 0,06251648
        +float2( -0.0000, +0.3750),	// 0,140625
        -float2( -0.0000, +0.3750),	// 0,140625
        +float2( +0.3536, -0.3536),	// 0,25006592
        -float2( +0.3536, -0.3536),	// 0,25006592
        +float2( -0.6250, -0.0000),	// 0,390625
        -float2( -0.6250, -0.0000),	// 0,390625
        +float2( +0.5303, +0.5303),	// 0,56243618
        -float2( +0.5303, +0.5303),	// 0,56243618
        +float2( -0.0000, -0.8750),	// 0,765625
        -float2( -0.0000, -0.8750),	// 0,765625
        +float2( -0.7071, +0.7071),	// 0,99998082
        -float2( -0.7071, +0.7071),	// 0,99998082
    };

    const float2 rotation = make_rotation(Shadow_GradientNoise()) * 2.5f / textureSize(pk_ShadowmapAtlas, 0).x;
    const float3 bias = Shadow_GetReceiverBias(uvz, index);
    
    float shadow = 0.0f;
    
    for (uint i = 0u; i < 16u; ++i)
    {
        const float2 offset = rotate2D(offsets[i], rotation);
        shadow += ShadowTest_Single(index, Shadow_CombineCoords(uvz, offset, bias));
    }

    return shadow / 16.0f;
}

float ShadowTest_Fast(const uint index, const float3 uvz)
{
    const float3 bias = Shadow_GetReceiverBias(uvz, index);
    return ShadowTest_Single(index, Shadow_CombineCoords(uvz, 0.0f.xx, bias));
}
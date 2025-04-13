#pragma once
#include "PostFXResources.glsl"

PK_DECLARE_SET_PASS uniform sampler2D pk_Bloom_Texture;
PK_DECLARE_SET_PASS uniform sampler2D pk_Bloom_LensDirtTex;

float3 Bloom(float3 color, float2 uv)
{
    const float2 tx = 1.0f / textureSize(pk_Bloom_Texture, 0).xy;

    float3 bloom = 0.0f.xxx;
    bloom += texture(pk_Bloom_Texture, uv + (float2(0, 0) - 0.5f.xx) * tx).rgb;
    bloom += texture(pk_Bloom_Texture, uv + (float2(0, 1) - 0.5f.xx) * tx).rgb;
    bloom += texture(pk_Bloom_Texture, uv + (float2(1, 1) - 0.5f.xx) * tx).rgb;
    bloom += texture(pk_Bloom_Texture, uv + (float2(1, 0) - 0.5f.xx) * tx).rgb;
    bloom *= 0.25f;

    float3 bloom_lens = 0.0f.xxx;
    bloom_lens += textureLod(pk_Bloom_Texture, uv + (float2(0, 0) - 0.5f.xx) * tx * 8.0f, 3).rgb;
    bloom_lens += textureLod(pk_Bloom_Texture, uv + (float2(0, 1) - 0.5f.xx) * tx * 8.0f, 3).rgb;
    bloom_lens += textureLod(pk_Bloom_Texture, uv + (float2(1, 1) - 0.5f.xx) * tx * 8.0f, 3).rgb;
    bloom_lens += textureLod(pk_Bloom_Texture, uv + (float2(1, 0) - 0.5f.xx) * tx * 8.0f, 3).rgb;
    bloom_lens *= 0.25f;

    const float lens_dirt = texture(pk_Bloom_LensDirtTex, uv).r;

    color = lerp(color, bloom, pk_Bloom_Intensity.xxx);
    color = lerp(color, bloom_lens, lens_dirt * pk_Bloom_DirtIntensity.xxx);

    return color;
}

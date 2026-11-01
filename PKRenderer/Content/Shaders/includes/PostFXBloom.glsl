#pragma once
#include "PostFXResources.glsl"

uniform Texture2D<float3> pk_Bloom_Texture;
uniform Texture2D<float> pk_Bloom_LensDirtTex;

float3 Bloom(float3 color, float2 uv)
{
    const float2 tx = 1.0f / pk_Bloom_Texture.GetSize();

    float3 bloom = 0.0f.xxx;
    bloom += pk_Bloom_Texture.SampleLevel(pk_SamplerTrilinearClamped, uv + (float2(0, 0) - 0.5f.xx) * tx, 0.0f).rgb;
    bloom += pk_Bloom_Texture.SampleLevel(pk_SamplerTrilinearClamped, uv + (float2(0, 1) - 0.5f.xx) * tx, 0.0f).rgb;
    bloom += pk_Bloom_Texture.SampleLevel(pk_SamplerTrilinearClamped, uv + (float2(1, 1) - 0.5f.xx) * tx, 0.0f).rgb;
    bloom += pk_Bloom_Texture.SampleLevel(pk_SamplerTrilinearClamped, uv + (float2(1, 0) - 0.5f.xx) * tx, 0.0f).rgb;
    bloom *= 0.25f;

    float3 bloom_lens = 0.0f.xxx;
    bloom_lens += pk_Bloom_Texture.SampleLevel(pk_SamplerTrilinearClamped, uv + (float2(0, 0) - 0.5f.xx) * tx * 8.0f, 3.0f).rgb;
    bloom_lens += pk_Bloom_Texture.SampleLevel(pk_SamplerTrilinearClamped, uv + (float2(0, 1) - 0.5f.xx) * tx * 8.0f, 3.0f).rgb;
    bloom_lens += pk_Bloom_Texture.SampleLevel(pk_SamplerTrilinearClamped, uv + (float2(1, 1) - 0.5f.xx) * tx * 8.0f, 3.0f).rgb;
    bloom_lens += pk_Bloom_Texture.SampleLevel(pk_SamplerTrilinearClamped, uv + (float2(1, 0) - 0.5f.xx) * tx * 8.0f, 3.0f).rgb;
    bloom_lens *= 0.25f;

    const float lens_dirt = pk_Bloom_LensDirtTex.SampleLevel(pk_SamplerBilinearClamped, uv, 0.0f).r;

    color = lerp(color, bloom, pk_View.Bloom_Intensity.xxx);
    color = lerp(color, bloom_lens, lens_dirt * pk_View.Bloom_DirtIntensity.xxx);

    return color;
}

#pragma once
#include SharedPostEffects.glsl

PK_DECLARE_SET_PASS uniform sampler2D pk_BloomTexture;
PK_DECLARE_SET_PASS uniform sampler2D pk_BloomLensDirtTex;

float3 Bloom(float3 color, float2 uv)
{
	const float2 tx = 1.0f / textureSize(pk_BloomTexture, 0).xy;

	float3 bloom = 0.0f.xxx;
	bloom += tex2D(pk_BloomTexture, uv + (float2(0, 0) - 0.5f.xx) * tx).rgb;
	bloom += tex2D(pk_BloomTexture, uv + (float2(0, 1) - 0.5f.xx) * tx).rgb;
	bloom += tex2D(pk_BloomTexture, uv + (float2(1, 1) - 0.5f.xx) * tx).rgb;
	bloom += tex2D(pk_BloomTexture, uv + (float2(1, 0) - 0.5f.xx) * tx).rgb;
	bloom *= 0.25f;

	float3 bloomLens = 0.0f.xxx;
	bloomLens += tex2DLod(pk_BloomTexture, uv + (float2(0, 0) - 0.5f.xx) * tx * 4.0f, 3).rgb;
	bloomLens += tex2DLod(pk_BloomTexture, uv + (float2(0, 1) - 0.5f.xx) * tx * 4.0f, 3).rgb;
	bloomLens += tex2DLod(pk_BloomTexture, uv + (float2(1, 1) - 0.5f.xx) * tx * 4.0f, 3).rgb;
	bloomLens += tex2DLod(pk_BloomTexture, uv + (float2(1, 0) - 0.5f.xx) * tx * 4.0f, 3).rgb;
	bloomLens *= 0.25f;

	const float lensdirt = tex2D(pk_BloomLensDirtTex, uv).r;

	color = lerp(color, bloom, pk_BloomIntensity.xxx);
	color = lerp(color, bloomLens * 0.333f, lensdirt * pk_BloomDirtIntensity.xxx);

	return color;
}
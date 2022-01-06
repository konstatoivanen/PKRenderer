#pragma once
#include SharedPostEffects.glsl

PK_DECLARE_SET_PASS uniform sampler2D pk_BloomTexture;
PK_DECLARE_SET_PASS uniform sampler2D pk_BloomLensDirtTex;

float3 Bloom(float3 color, float2 uv)
{
	float3 lensdirt = tex2D(pk_BloomLensDirtTex, uv).rrr;

	float3 b0 = tex2DLod(pk_BloomTexture, uv, 0.0f).rgb;
	float3 b1 = tex2DLod(pk_BloomTexture, uv, 1.0f).rgb;
	float3 b2 = tex2DLod(pk_BloomTexture, uv, 2.0f).rgb;
	float3 b3 = tex2DLod(pk_BloomTexture, uv, 3.0f).rgb;
	float3 b4 = tex2DLod(pk_BloomTexture, uv, 4.0f).rgb;
	float3 b5 = tex2DLod(pk_BloomTexture, uv, 5.0f).rgb;

	// Optional curves. Removing weights from these will provide smoother transitions between bloom layers.
	float3 bloom = b0 * 0.5f + b1 * 0.6f + b2 * 0.6f + b3 * 0.45f + b4 * 0.35f + b5 * 0.23f;
	float3 bloomLens = b0 * 1.0f + b1 * 0.8f + b2 * 0.6f + b3 * 0.45f + b4 * 0.35f + b5 * 0.23f;
	
	bloom /= 2.2f;
	bloomLens /= 3.2f;
	
	color = lerp(color, bloom, pk_BloomIntensity.xxx);
	color = lerp(color, bloomLens, saturate(lensdirt * pk_BloomDirtIntensity));

	return color;
}
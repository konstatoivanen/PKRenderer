#pragma once
#include PostFXResources.glsl

PK_DECLARE_SET_PASS uniform sampler2D pk_Bloom_Texture;
PK_DECLARE_SET_PASS uniform sampler2D pk_Bloom_LensDirtTex;

vec4 cubic(float x)
{
    float x2 = x * x;
    float x3 = x2 * x;
    vec4 w;
    w.x =   -x3 + 3.0*x2 - 3.0*x + 1.0;
    w.y =  3.0*x3 - 6.0*x2       + 4.0;
    w.z = -3.0*x3 + 3.0*x2 + 3.0*x + 1.0;
    w.w =  x3;
    return w / 6.0;
}

vec3 bloomSampleBicubic(vec2 coord, int l)
{
	vec2 resolution = textureSize(pk_Bloom_Texture, l).xy;

	coord *= resolution;

	float fx = fract(coord.x);
    float fy = fract(coord.y);
    coord.x -= fx;
    coord.y -= fy;

    fx -= 0.5;
    fy -= 0.5;

    vec4 xcubic = cubic(fx);
    vec4 ycubic = cubic(fy);

    vec4 c = vec4(coord.x - 0.5, coord.x + 1.5, coord.y - 0.5, coord.y + 1.5);
    vec4 s = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);
    vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;

    vec3 sample0 = tex2DLod(pk_Bloom_Texture, vec2(offset.x, offset.z) / resolution, l).rgb;
    vec3 sample1 = tex2DLod(pk_Bloom_Texture, vec2(offset.y, offset.z) / resolution, l).rgb;
    vec3 sample2 = tex2DLod(pk_Bloom_Texture, vec2(offset.x, offset.w) / resolution, l).rgb;
    vec3 sample3 = tex2DLod(pk_Bloom_Texture, vec2(offset.y, offset.w) / resolution, l).rgb;

    float sx = s.x / (s.x + s.y);
    float sy = s.z / (s.z + s.w);

    return mix( mix(sample3, sample2, sx), mix(sample1, sample0, sx), sy);
}

float3 Bloom(float3 color, float2 uv)
{
	const float2 tx = 1.0f / textureSize(pk_Bloom_Texture, 0).xy;
	const float lensdirt = tex2D(pk_Bloom_LensDirtTex, uv).r;

	for (uint i = 0u; i < 4; ++i)
	{
		const float iw = 1.0f / (6.0f - i);
		const float2 mip_tx = tx * exp2(i);

		float3 bloom = bloomSampleBicubic(uv, int(i)).rgb * iw;// 0.0f.xxx;
		float3 bloomLens = bloomSampleBicubic(uv, int(i)).rgb * iw;

		color = lerp(color, bloom, pk_Bloom_Intensity[i]);
		color = lerp(color, bloomLens, lensdirt * pk_Bloom_DirtIntensity[i]);
	}

	return color;
}
#pragma once
#ifndef PK_NOISE
#define PK_NOISE

// Source: https://www.shadertoy.com/view/4sSXDW
float NoiseGrain0(const float2 n, const float x) { return fract(sin(dot(n.xy + x.xx, float2(12.9898f, 78.233f))) *  43758.5453f); }

float NoiseGrain1(const float2 n, const float x)
{
    const float b = 2.0, c = -12.0;
    return (1.0 / (4.0 + b * 4.0 + abs(c))) * 
    (
        NoiseGrain0(n + float2(-1.0, -1.0), x) +
        NoiseGrain0(n + float2( 0.0, -1.0), x) * b +
        NoiseGrain0(n + float2( 1.0, -1.0), x) +
        NoiseGrain0(n + float2(-1.0,  0.0), x) * b +
        NoiseGrain0(n + float2( 0.0,  0.0), x) * c +
        NoiseGrain0(n + float2( 1.0,  0.0), x) * b +
        NoiseGrain0(n + float2(-1.0,  1.0), x) +
        NoiseGrain0(n + float2( 0.0,  1.0), x) * b +
        NoiseGrain0(n + float2( 1.0,  1.0), x)
    );
}

float NoiseGrain2(const float2 n, const float x)
{
    const float b = 2.0, c = 4.0;
    return (1.0 / (4.0 + b * 4.0 + abs(c))) * 
    (
        NoiseGrain1(n + float2(-1.0, -1.0), x) +
        NoiseGrain1(n + float2( 0.0, -1.0), x) * b +
        NoiseGrain1(n + float2( 1.0, -1.0), x) +
        NoiseGrain1(n + float2(-1.0,  0.0), x) * b +
        NoiseGrain1(n + float2( 0.0,  0.0), x) * c +
        NoiseGrain1(n + float2( 1.0,  0.0), x) * b +
        NoiseGrain1(n + float2(-1.0,  1.0), x) +
        NoiseGrain1(n + float2( 0.0,  1.0), x) * b +
        NoiseGrain1(n + float2( 1.0,  1.0), x)
    );
}

float NoiseGrainBW(const float2 n, const float x) { return NoiseGrain2(n, fract(x)); }

float3 NoiseGrainColor(const float2 uv, const float x)
{
    return float3(NoiseGrain2(uv, 0.07 * fract(x)), NoiseGrain2(uv, 0.11 * fract(x)), NoiseGrain2(uv, 0.13 * fract(x)));
}

float2 NoiseCell(const int2 p)
{
	int i = p.y * 256 + p.x;
    int2 n = int2(i, i + 57);
    n = (n << 13) ^ n;
	n = (n * (n * n * 15731 + 789221) + 1376312589) & 2147483647;
    return (n / 2147483647.0f) - 0.5f; 
}

// Range 0 - 1
// Input must be saturated
float NoiseUniformToTriangle(float n)
{
	const float o = n * 2.0 - 1.0;
    n = max(-1.0f, o * inversesqrt(abs(o))) - sign(o) + 0.5f;
	return (n / 1.5f) + 0.5f;
}

float NoiseTriangle(const float3 n)
{
	const float t = fract(n.z);
	const float r = fract(sin(dot(n.xy + 0.07 * t, float2(12.9898, 78.233))) * 43758.5453);
	return NoiseUniformToTriangle(r);
}

float NoiseP(const float3 x)
{
	float3 p = floor(x);
	float3 f = fract(x);
	f = f * f * (3.0f - 2.0f * f);
	float n = p.x + p.y * 157.0f + 113.0f * p.z;
    float4 na = fract(sin(n + float4(0.0f, 1.0f, 157.0f, 158.0f)) * 753.5453123f); 
    float4 nb = fract(sin(n + float4(113.0f, 114.0f, 270.0f, 271.0f)) * 753.5453123f); 
    na = lerp(na, nb, f.z);
    na.xy = lerp(na.xy, na.zw, f.y);
    return lerp(na.x, na.y, f.x);
}

float NoiseScroll(const float3 pos, const float time, const float scale, const float3 dir, const float amount, const float bias, const float mult)
{
	float3 noiseScroll = dir * time;
	float3 q = (pos - noiseScroll) * scale;
	float  f = 0.5f * NoiseP(q);
	
    // scroll the next octave in the opposite direction to get some morphing instead of just scrolling
	q += noiseScroll * scale;
	q  = q * 2.01f;
	f += 0.25f * NoiseP(q);

    return lerp(1.0f, max((f + bias) * mult, 0.0f), amount);
}

#endif
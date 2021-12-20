#pragma once
#ifndef PK_NOISE
#define PK_NOISE

#include HLSLSupport.glsl

float NoiseUV(float u, float v) { return fract(43758.5453 * sin(dot(float2(12.9898, 78.233), float2(u, v)))); }

float NoiseGradient(float2 uv, float2 res) { return fract(52.9829189f * fract(dot(float2(0.06711056f, 0.00583715f), floor(uv * res)))); }

float NoiseWanghash(float3 seedvec)
{
    uint u = floatBitsToUint(seedvec.x);
    uint v = floatBitsToUint(seedvec.y);
    uint s = floatBitsToUint(seedvec.z);
    
    uint seed = (u * 1664525u + v) + s;

    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2d;
    seed = seed ^ (seed >> 15u);

    float value = float(seed);

    value *= (1.0 / 4294967296.0);

    return value;
}

int NoiseIhash(int n)
{
	n = (n << 13) ^ n;
	return (n * (n * n * 15731 + 789221) + 1376312589) & 2147483647;
}

float NoiseFhash(float n) { return fract(sin(n) * 753.5453123); }

float NoiseFrand(int n) { return NoiseIhash(n) / 2147483647.0; }

// Source: https://www.shadertoy.com/view/4sSXDW
float NoiseGrain0(float2 n, float x) { return fract(sin(dot(n.xy + x.xx, float2(12.9898f, 78.233f))) *  43758.5453f); }

float NoiseGrain1(float2 n, float x)
{
    float b = 2.0, c = -12.0;
    return (1.0 / (4.0 + b * 4.0 + abs(c))) * (
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

float NoiseGrain2(float2 n, float x)
{
    float b = 2.0, c = 4.0;
    return (1.0 / (4.0 + b * 4.0 + abs(c))) * (
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

float NoiseGrainBW(float2 n, float x) { return NoiseGrain2(n, fract(x)); }

float3 NoiseGrainColor(float2 uv, float x)
{
    return float3(NoiseGrain2(uv, 0.07 * fract(x)), 
                  NoiseGrain2(uv, 0.11 * fract(x)), 
                  NoiseGrain2(uv, 0.13 * fract(x)));
}

float2 NoiseCell(int2 p)
{
	int i = p.y * 256 + p.x;
	return float2(NoiseFrand(i), NoiseFrand(i + 57)) - 0.5;//*2.0-1.0;
}

float NoiseUniformToTriangle(float n)
{
	float orig = n * 2.0 - 1.0;
	n = orig * inversesqrt(abs(orig));
	n = max(-1.0, n); 
	n = n - sign(orig) + 0.5;
	return n;
}

float NoiseTriangle(float3 n)
{
	float t = fract(n.z);
	float nrnd0 = fract(sin(dot(n.xy + 0.07 * t, vec2(12.9898, 78.233))) * 43758.5453);
	return NoiseUniformToTriangle(nrnd0);
}

float NoiseP(float3 x)
{
	float3 p = floor(x);
	float3 f = fract(x);
	f = f * f * (3.0 - 2.0 * f);

	float n = p.x + p.y * 157.0 + 113.0 * p.z;
	return lerp(lerp(lerp(NoiseFhash(n + 0.0), NoiseFhash(n + 1.0), f.x),
		   lerp(NoiseFhash(n + 157.0), NoiseFhash(n + 158.0), f.x), f.y),
		   lerp(lerp(NoiseFhash(n + 113.0), NoiseFhash(n + 114.0), f.x),
			    lerp(NoiseFhash(n + 270.0), NoiseFhash(n + 271.0), f.x), f.y), f.z);
}

float NoiseScroll(float3 pos, float time, float scale, float3 dir, float amount, float bias, float mult)
{
	float noiseScale = scale;
	float3 noiseScroll = dir * time;
	float3 q = pos - noiseScroll;
	q *= scale;
	float f = 0;
	f = 0.5 * NoiseP(q);
	// scroll the next octave in the opposite direction to get some morphing instead of just scrolling
	q += noiseScroll * scale;
	q = q * 2.01;
	f += 0.25 * NoiseP(q);

	f += bias;
	f *= mult;

	f = max(f, 0.0);
	return lerp(1.0, f, amount);
}

float bayermatrix[4][4] =
{
	{ 0.0f,    0.5f,    0.125f,  0.625f},
	{ 0.75f,   0.22f,   0.875f,  0.375f},
	{ 0.1875f, 0.6875f, 0.0625f, 0.5625},
	{ 0.9375f, 0.4375f, 0.8125f, 0.3125}
};

float NoiseBayerMatrix(uint2 pxcoord)
{
	return bayermatrix[pxcoord.x % 4][pxcoord.y % 4];
}

#endif
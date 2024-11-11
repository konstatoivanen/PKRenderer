#pragma once
#ifndef PK_NOISE
#define PK_NOISE

// Source: https://www.shadertoy.com/view/4sSXDW
// Changed 78.233f to 88.233f to fix visible banding.
float NoiseGrain0(const float2 n, const float x) { return fract(sin(dot(n.xy + x.xx, float2(12.9898f, 88.233f))) *  43758.5453f); }

float NoiseGrain1(const float2 n, const float x, const float range)
{
    const float b = 2.0, c = -12.0;
    return (1.0 / (4.0 + b * 4.0 + abs(c))) * 
    (
        NoiseGrain0(mod(n + float2(-1.0, -1.0), range.xx), x) +
        NoiseGrain0(mod(n + float2( 0.0, -1.0), range.xx), x) * b +
        NoiseGrain0(mod(n + float2( 1.0, -1.0), range.xx), x) +
        NoiseGrain0(mod(n + float2(-1.0,  0.0), range.xx), x) * b +
        NoiseGrain0(mod(n + float2( 0.0,  0.0), range.xx), x) * c +
        NoiseGrain0(mod(n + float2( 1.0,  0.0), range.xx), x) * b +
        NoiseGrain0(mod(n + float2(-1.0,  1.0), range.xx), x) +
        NoiseGrain0(mod(n + float2( 0.0,  1.0), range.xx), x) * b +
        NoiseGrain0(mod(n + float2( 1.0,  1.0), range.xx), x)
    );
}

float NoiseGrain2(const float2 n, const float x, const float range)
{
    const float b = 2.0, c = 4.0;
    return (1.0 / (4.0 + b * 4.0 + abs(c))) * 
    (
        NoiseGrain1(n + float2(-1.0, -1.0), x, range) +
        NoiseGrain1(n + float2( 0.0, -1.0), x, range) * b +
        NoiseGrain1(n + float2( 1.0, -1.0), x, range) +
        NoiseGrain1(n + float2(-1.0,  0.0), x, range) * b +
        NoiseGrain1(n + float2( 0.0,  0.0), x, range) * c +
        NoiseGrain1(n + float2( 1.0,  0.0), x, range) * b +
        NoiseGrain1(n + float2(-1.0,  1.0), x, range) +
        NoiseGrain1(n + float2( 0.0,  1.0), x, range) * b +
        NoiseGrain1(n + float2( 1.0,  1.0), x, range)
    );
}

float NoiseGrainBW(const float2 n, const float x, const float range) { return NoiseGrain2(n, fract(x), range) * 10.2f; }

float3 NoiseGrainColor(const float2 uv, const float x, const float range)
{
    return float3(NoiseGrain2(uv, 0.07 * fract(x), range), NoiseGrain2(uv + 17.0f.xx, 0.11 * fract(x), range), NoiseGrain2(uv + 29.0f.xx, 0.13 * fract(x), range)) * 10.2f;
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
    return (n / 3.0f) + 0.5f;
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

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

float2 Hammersley(uint i, uint N)
{
    return float2(float(i % N) / float(N), RadicalInverse_VdC(i));
}

float InterleavedGradientNoise(float2 coord, uint frame)
{
    // "Interleaved gradient noise", by Jorge Jimenez,
    // http://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare
    frame = frame & 63u; // need to periodically reset frame to avoid numerical issues
    float x = coord.x + 5.588238f * float(frame);
    float y = coord.y + 5.588238f * float(frame);
    return fract(52.9829189f * fract(0.06711056f * x + 0.00583715f * y));
}

#endif

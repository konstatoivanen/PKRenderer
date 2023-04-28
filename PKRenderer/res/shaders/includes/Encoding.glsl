#pragma once
#include Utilities.glsl

const float3x3 PK_RGBToYCoCg = float3x3(0.25, 0.5, -0.25, 0.5, 0.0, 0.5, 0.25, -0.5, -0.25);
const float3x3 PK_YCoCgToRGB = float3x3(1.0, 1.0, 1.0, 1.0, 0.0, -1.0, -1.0, 1.0, -1.0);
const float3x3 PK_RGBToYCoCgR = float3x3(0.25, 1.0, -0.5, 0.5, 0.0, 1.0, 0.25, -1.0, -0.5);
const float3x3 PK_YCoCgRToRGB = float3x3(1.0, 1.0, 1.0, 0.5, 0.0, -0.5, -0.5, 0.5, -0.5);

#define HDRFactor 8.0

float4 HDREncode(float3 color) 
{
    color /= HDRFactor;
    float alpha = ceil(max(max(color.r, color.g), color.b) * 255.0) / 255.0;
    return float4(color / alpha, alpha);
}

float3 HDRDecode(float4 hdr) { return float3(hdr.rgb * hdr.a * HDRFactor); }

float2 OctaWrap(float2 v) { return (1.0 - abs(v.yx)) * float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0); }

float2 OctaEncode(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xz = n.y >= 0.0 ? n.xz : OctaWrap(n.xz);
    n.xz = n.xz * 0.5 + 0.5;
    return n.xz;
}

//Source: https://twitter.com/Stubbesaurus/status/937994790553227264
float3 OctaDecode(float2 f)
{
    f = f * 2.0f - 1.0f;
    float3 n = float3(f.x, 1.0f - abs(f.x) - abs(f.y), f.y);
    float t = max(-n.y, 0.0);
    n.x += n.x >= 0.0f ? -t : t;
    n.z += n.z >= 0.0f ? -t : t;
    return normalize(n);
}

float2 OctaUV(float3 offset, float3 direction) { return offset.xy + OctaEncode(direction) * offset.z; }

float2 OctaUV(float3 direction) { return OctaEncode(direction); }

float2 CylinderUV(float3 direction)
{
    float angleh = (atan(direction.x, direction.z) + 3.14159265359f) * 0.15915494309f;
	float anglev = acos(dot(direction, float3(0, 1, 0))) * 0.31830988618f;
	return float2(angleh, anglev);
}

float3 RGBToHSV(float3 c)
{
	float4 k = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	float4 p = lerp(float4(c.bg, k.wz), float4(c.gb, k.xy), step(c.b, c.g));
	float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));

	float d = q.x - min(q.w, q.y);
	float e = 1.0e-10f;

	return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 HSVToRGB(float3 c)
{
	float4 k = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	float3 p = abs(fract(c.xxx + k.xyz) * 6.0 - k.www);
	return c.zzz * lerp(k.xxx, saturate(p - k.xxx), c.y);
}

float3 HSVToRGB(float hue, float saturation, float value)
{
    return HSVToRGB(float3(hue, saturation, value));
}

uint EncodeE5GR9(float2 v)
{
    const int N = 9;
    const int Np2 = 1 << N;
    const int B = 15;

    v = clamp(v, float2(0.0), float2(65408));
    float max_c = max(v.x, v.y);

    // for log2
    if (max_c == 0.0)
    {
        return 0;
    }

    int exp_shared_p = max(-B-1, int(floor(log2(max_c)))) + 1 + B;
    int max_s = int(round(max_c * exp2(-float(exp_shared_p - B - N))));
    int exp_shared = max_s != Np2 ? exp_shared_p : exp_shared_p + 1;

    float s = exp2(-float(exp_shared - B - N));
    uint2 rgb_s = uint2(round(v * s));

    return (exp_shared << (3 * 9)) | (rgb_s.y << (2 * 9)) | (rgb_s.x << (1 * 9));
}

float2 DecodeE5GR9(const uint v)
{
    int exp_shared = int(v >> (3 * 9));
    float s = exp2(float(exp_shared - 15 - 9));
    return s * float2((v >> (1 * 9)) & ((1 << 9) - 1), (v >> (2 * 9)) & ((1 << 9) - 1));
}
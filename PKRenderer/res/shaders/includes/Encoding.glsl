#pragma once
#include HLSLSupport.glsl

#define HDRFactor 8.0

float4 HDREncode(float3 color) 
{
    color /= HDRFactor;
    float alpha = ceil( max(max(color.r, color.g), color.b) * 255.0) / 255.0;
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
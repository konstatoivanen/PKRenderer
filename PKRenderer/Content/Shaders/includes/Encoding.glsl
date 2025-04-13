#pragma once
#include "Utilities.glsl"

float2 OctaWrap(float2 v) 
{ 
    return (1.0 - abs(v.yx)) * float2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0); 
}

float2 EncodeOctaUv(float3 n)
{
    n /= (abs(n.x) + abs(n.y) + abs(n.z));
    n.xz = lerp(OctaWrap(n.xz), n.xz, (n.y >= 0.0f).xx);
    n.xz = n.xz * 0.5f + 0.5f;
    return n.xz;
}

//Source: https://twitter.com/Stubbesaurus/status/937994790553227264
float3 DecodeOctaUv(float2 f)
{
    f = f * 2.0f - 1.0f;
    float3 n = float3(f.x, 1.0f - abs(f.x) - abs(f.y), f.y);
    float t = max(-n.y, 0.0f);
    n.x += lerp(t, -t, n.x >= 0.0f);
    n.z += lerp(t, -t, n.z >= 0.0f);
    return normalize(n);
}

float2 EncodeOctaUv(float3 offset, float3 direction) { return offset.xy + EncodeOctaUv(direction) * offset.z; }

uint EncodeOctaUv2x16(float3 direction) { return packUnorm2x16(EncodeOctaUv(direction)); }

float3 DecodeOctaUv2x16(uint packed) { return DecodeOctaUv(unpackUnorm2x16(packed)); }

float2 EncodeCylinderUv(float3 direction)
{
    const float angle_h = (atan(direction.x, direction.z) + 3.14159265359f) * 0.15915494309f;
    const float angle_v = acos(dot(direction, float3(0, 1, 0))) * 0.31830988618f;
    return float2(angle_h, angle_v);
}

float3 RgbToHsv(float3 c)
{
    const float4 k = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    const float4 p = lerp(float4(c.bg, k.wz), float4(c.gb, k.xy), step(c.b, c.g));
    const float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));
    const float d = q.x - min(q.w, q.y);
    const float e = 1.0e-10f;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 HsvToRgb(float3 c)
{
    const float4 k = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    const float3 p = abs(fract(c.xxx + k.xyz) * 6.0 - k.www);
    return c.zzz * lerp(k.xxx, saturate(p - k.xxx), c.y);
}

float3 HsvToRgb(float hue, float saturation, float value) { return HsvToRgb(float3(hue, saturation, value)); }

//Source: https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_shared_exponent.txt
uint EncodeE5BGR9(float3 unpacked)
{
    unpacked = clamp(unpacked, 0.0f, 65408.0f);
    const float max_c = max(unpacked.r, max(unpacked.g, unpacked.b));

    // for log2
    if (max_c == 0.0f)
    {
        return 0u;
    }

    const int exp_shared_p = max(-16, int(floor(log2(max_c)))) + 16;
    const int max_s = int(round(max_c * exp2(-exp_shared_p + 24)));
    const int exp_shared = exp_shared_p + int(max_s == 512);
    const uint3 rgb = uint3(round(unpacked * exp2(-exp_shared + 24)));
    return (uint(exp_shared) << 27u) | (rgb.z << 18u) | (rgb.y << 9u) | rgb.x;
}

float3 DecodeE5BGR9(const uint p) { return float3(p & 511u, (p >> 9u) & 511u, (p >> 18u) & 511u) * exp2(int(p >> 27u) - 24); }

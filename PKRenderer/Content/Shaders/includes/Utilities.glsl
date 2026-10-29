#pragma once
#ifndef PK_UTILITIES
#define PK_UTILITIES

#pragma pk_log_verbose

float pow2(float x) { return x * x; }
float pow3(float x) { return x * x * x; }
float pow4(float x) { return x * x * x * x; }
float pow5(float x) { return x * x * x * x * x; }

half pow2(half x) { return x * x; }
half pow3(half x) { return x * x * x; }
half pow4(half x) { return x * x * x * x; }
half pow5(half x) { return x * x * x * x * x; }

float cmin(float2 x) { return min(x.x, x.y); }
float cmin(float3 x) { return min(min(x.x, x.y), x.z); }
float cmin(float4 x) { return min(min(min(x.x, x.y), x.z), x.w); }
float cmax(float2 x) { return max(x.x, x.y); }
float cmax(float3 x) { return max(max(x.x, x.y), x.z); }
float cmax(float4 x) { return max(max(max(x.x, x.y), x.z), x.w); }

half cmin(half2 x) { return min(x.x, x.y); }
half cmin(half3 x) { return min(min(x.x, x.y), x.z); }
half cmin(half4 x) { return min(min(min(x.x, x.y), x.z), x.w); }
half cmax(half2 x) { return max(x.x, x.y); }
half cmax(half3 x) { return max(max(x.x, x.y), x.z); }
half cmax(half4 x) { return max(max(max(x.x, x.y), x.z), x.w); }

T mod<T : __BuiltinFloatingPointType>(T x, T y) { return x - y * floor(x / y); }
vector<T, N> mod<T : __BuiltinFloatingPointType, let N : int>(vector<T, N> x, vector<T, N> y) { return x - y * floor(x / y); }

uint packHalf2x16(float2 v) { uint2 h = f32tof16(v); return h.x | (h.y << 16u); }
float2 unpackHalf2x16(uint v) { return f16tof32(uint2(v & 0xFFFFu, v >> 16u)); }
uint2 packHalf4x16(float4 v) { return uint2(packHalf2x16(v.xy), packHalf2x16(v.zw)); }
float4 unpackHalf4x16(uint2 v) { return float4(unpackHalf2x16(v.x), unpackHalf2x16(v.y)); }

uint packUnorm2x16(float2 v) { uint2 u = uint2(round(saturate(v) * 65535.0f)); return (u.x & 0xFFFFu) | (u.y << 16u); }
float2 unpackUnorm2x16(uint p) { return float2(p & 0xFFFFu, p >> 16u) * (1.0f / 65535.0f); }

uint packUnorm4x8(float4 v) { uint4 u = uint4(round(saturate(v) * 255.0f)); return (u.x & 0xFFu) | ((u.y & 0xFFu) << 8u) | ((u.z & 0xFFu) << 16u) | ((u.w & 0xFFu) << 24u); }
float4 unpackUnorm4x8(uint p) { return float4(p & 0xFFu, (p >> 8u) & 0xFFu, (p >> 16u) & 0xFFu, p >> 24u) * (1.0f / 255.0f); }
float4 unpackSnorm4x8(uint p) { int4 s = int4(int(p << 24u) >> 24, int(p << 16u) >> 24, int(p << 8u) >> 24, int(p) >> 24); return clamp(float4(s) * (1.0f / 127.0f), -1.0f, 1.0f); }

uint WaveBallotBitCount(uint4 mask) { uint4 c = countbits(mask); return c.x + c.y + c.z + c.w; }

float AsUnorm(uint x) { return asfloat((x & 0x007fffffu) | 0x3f800000u) - 1.0f; }
uint Checkerboard(uint2 coord, uint offset) { return ((coord.x ^ coord.y) ^ offset) & 0x1u; }

float SafePositiveRcp(float f) { return select(f <= 1e-12f, 0.0f, rcp(f)); }
float3 SafeNormalize(float3 v) { return v * SafePositiveRcp(length(v)); }
float4 NormalizeLength(float3 v) { float l = length(v); return float4(v.xyz * SafePositiveRcp(l), l); }

float4 Mul3x3(const float3x3 matrix, const float4 v) { return float4(mul(matrix, v.xyz), v.w); }
float4 Mul3x3(const float4 v, const float3x3 matrix) { return float4(mul(v.xyz, matrix), v.w); }

half2 RadianRotation(half radian) { return half2(cos(radian), sin(radian)); }
float2 RadianRotation(float radian) { return float2(cos(radian), sin(radian)); }
half2 MulRotation(half2 v, half2 r) { return half2(v.x * r.x - v.y * r.y, v.x * r.y + v.y * r.x); }
float2 MulRotation(float2 v, float2 r) { return float2(v.x * r.x - v.y * r.y, v.x * r.y + v.y * r.x); }

// Source: https://graphics.pixar.com/library/OrthonormalB/paper.pdf
void CreateTBN(const float3 n, out float3 b1, out float3 b2)
{
    const float s = n.z >= 0.0f ? 1.0f : -1.0f;
    const float a = -1.0f / (s + n.z);
    const float b = n.x * n.y * a;
    b1 = float3(1.0f + s * n.x * n.x * a, s * b, -s * n.x);
    b2 = float3(b, s + n.y * n.y * a, -n.y);
}

float2x3 CreateTB(const float3 n, float scale) { float3 t, b; CreateTBN(n, t, b); return float2x3(t * scale, b * scale); }
float3x3 CreateTBN(const float3 n) { float3 t, b; CreateTBN(n,t,b); return float3x3(t, b, n); }

#define lerp_sat(a,b,c) lerp(a,b,saturate(c))
#define lerp_outquad(a,b,c) (-((b) - (a)) * (c) * ((c) - 2) + (a))
#define lerp_inquad(a,b,c) (((b) - (a)) * (c) * (c) + (a))
#define unlerp(a,b,value) (((value) - (a)) / ((b) - (a)))
#define unlerp_sat(a,b,value) saturate(((value) - (a)) / ((b) - (a)))

#define Replace_NaN(v, r) (any(isnan(v)) ? (r) : (v))
#define Test_NaN_EPS4(v) (isnan(v) || (v) <= 1e-4f)
#define Test_NaN_EPS6(v) (isnan(v) || (v) <= 1e-6f)
#define Test_EPS4(v) ((v) <= 1e-4f)
#define Test_EPS6(v) ((v) <= 1e-6f)

#endif

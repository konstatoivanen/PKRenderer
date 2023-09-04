#pragma once
#include Utilities.glsl

const float3x3 PK_RGBToYCoCg = float3x3(0.25, 0.5, -0.25, 0.5, 0.0, 0.5, 0.25, -0.5, -0.25);
const float3x3 PK_YCoCgToRGB = float3x3(1.0, 1.0, 1.0, 1.0, 0.0, -1.0, -1.0, 1.0, -1.0);
const float3x3 PK_RGBToYCoCgR = float3x3(0.25, 1.0, -0.5, 0.5, 0.0, 1.0, 0.25, -1.0, -0.5);
const float3x3 PK_YCoCgRToRGB = float3x3(1.0, 1.0, 1.0, 0.5, 0.0, -0.5, -0.5, 0.5, -0.5);

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
    const float angleh = (atan(direction.x, direction.z) + 3.14159265359f) * 0.15915494309f;
	const float anglev = acos(dot(direction, float3(0, 1, 0))) * 0.31830988618f;
	return float2(angleh, anglev);
}

float3 RGBToHSV(float3 c)
{
	const float4 k = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
	const float4 p = lerp(float4(c.bg, k.wz), float4(c.gb, k.xy), step(c.b, c.g));
	const float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));
	const float d = q.x - min(q.w, q.y);
	const float e = 1.0e-10f;
	return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float3 HSVToRGB(float3 c)
{
	const float4 k = float4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
	const float3 p = abs(fract(c.xxx + k.xyz) * 6.0 - k.www);
	return c.zzz * lerp(k.xxx, saturate(p - k.xxx), c.y);
}

float3 HSVToRGB(float hue, float saturation, float value) { return HSVToRGB(float3(hue, saturation, value)); }

uint EncodeOctaUV(float3 direction) { return packUnorm2x16(OctaUV(direction)); }
float3 DecodeOctaUV(uint packed) { return OctaDecode(unpackUnorm2x16(packed)); }

//Source: https://www.khronos.org/registry/OpenGL/extensions/EXT/EXT_texture_shared_exponent.txt
#define ENCODE_E5BGR9_EXPONENT_BITS 5
#define ENCODE_E5BGR9_MANTISSA_BITS 9
#define ENCODE_E5BGR9_MAX_VALID_BIASED_EXP 31
#define ENCODE_E5BGR9_EXP_BIAS 15
#define ENCODE_E5BGR9_MANTISSA_VALUES (1 << 9)
#define ENCODE_E5BGR9_MANTISSA_MASK (ENCODE_E5BGR9_MANTISSA_VALUES - 1)
#define ENCODE_E5BGR9_SHAREDEXP_MAX 65408

uint EncodeE5BGR9(float3 unpacked)
{
    const int N = ENCODE_E5BGR9_MANTISSA_BITS;
    const int Np2 = 1 << N;
    const int B = ENCODE_E5BGR9_EXP_BIAS;

    unpacked = clamp(unpacked, 0.0f.xxx, ENCODE_E5BGR9_SHAREDEXP_MAX.xxx);
    float max_c = max(unpacked.r, max(unpacked.g, unpacked.b));

    // for log2
    if (max_c == 0.0)
    {
        return 0;
    }

    const int exp_shared_p = max(-B-1, int(floor(log2(max_c)))) + 1 + B;
    const int max_s = int(round(max_c * exp2(-float(exp_shared_p - B - N))));
    const int exp_shared = max_s != Np2 ? exp_shared_p : exp_shared_p + 1;
    const float s = exp2(-float(exp_shared - B - N));
    const uint3 rgb_s = uint3(round(unpacked * s));

    return (exp_shared << (3 * ENCODE_E5BGR9_MANTISSA_BITS)) |
           (rgb_s.b    << (2 * ENCODE_E5BGR9_MANTISSA_BITS)) |
           (rgb_s.g    << (1 * ENCODE_E5BGR9_MANTISSA_BITS)) |
           (rgb_s.r);
}

float3 DecodeE5BGR9(const uint _packed)
{
    const int N = ENCODE_E5BGR9_MANTISSA_BITS;
    const int B = ENCODE_E5BGR9_EXP_BIAS;

    int exp_shared = int(_packed >> (3 * ENCODE_E5BGR9_MANTISSA_BITS));
    float s = exp2(float(exp_shared - B - N));

    return s * float3(
        (_packed                                       ) & ENCODE_E5BGR9_MANTISSA_MASK, 
        (_packed >> (1 * ENCODE_E5BGR9_MANTISSA_BITS)) & ENCODE_E5BGR9_MANTISSA_MASK,
        (_packed >> (2 * ENCODE_E5BGR9_MANTISSA_BITS)) & ENCODE_E5BGR9_MANTISSA_MASK
    );
}

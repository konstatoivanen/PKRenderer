#pragma once
#include Constants.glsl

// Source: https://developer.download.nvidia.com/video/gputechconf/gtc/2019/presentation/s9985-exploring-ray-traced-future-in-metro-exodus.pdf
struct SH
{
	float4 Y;
	float2 CoCg;
};

#define pk_ZeroSH SH(0.0f.xxxx, 0.0f.xx)

float4 SH_GetBasis(const float3 d) { return float4(1.0f, d.yzx) * PK_L1BASIS; }
SH SH_Interpolate(const SH sha, const SH shb, const float i) { return SH(lerp(sha.Y, shb.Y, i), lerp(sha.CoCg, shb.CoCg, i)); }
SH SH_Scale(const SH sh, float s) { return SH(sh.Y * s, sh.CoCg * s); }
SH SH_Add(const SH sha, const SH shb, float sb) { return SH(sha.Y + shb.Y * sb, sha.CoCg + shb.CoCg * sb); }

float3 SH_ToIrradiance(const SH sh, const float3 dir, const float chromaBias)
{
	float Y = max(0.0f, 2.0f * (PK_L1BASIS_COSINE.y * dot(sh.Y.wyz, dir) + PK_L1BASIS_COSINE.x * sh.Y.x));
	float2 CoCg = sh.CoCg * PK_L1BASIS.x * Y / (sh.Y.x + 1e-6f);
	
	Y *= 1.0f - chromaBias;
	CoCg *= 1.0f + chromaBias;

	const float T = Y - CoCg.y * 0.5f;
	const float G = CoCg.y + T;
	const float B = T - CoCg.x * 0.5f;
	const float R = B + CoCg.x;
	return float3(R, G, B);
}

float3 SH_ToIrradiance(const SH sh, const float3 d) { return SH_ToIrradiance(sh, d, 0.0f); }

float3 SH_ToColor(const SH sh) 
{
	const float Y = sh.Y.x / PK_L1BASIS.x;
	const float T = Y - sh.CoCg.y * 0.5f;
	const float G = sh.CoCg.y + T;
	const float B = T - sh.CoCg.x * 0.5f;
	const float R = B + sh.CoCg.x;
	return float3(R, G, B);
}

float SH_ToLuminance(const SH sh, const float3 d) { return dot(PK_LUMA_BT709, SH_ToIrradiance(sh, d)); }
float SH_ToLuminance(const SH sh, const float3 d, const float chromaBias) { return dot(PK_LUMA_BT709, SH_ToIrradiance(sh, d, chromaBias)); }
float SH_ToLuminanceL0(const SH sh) { return sh.Y.x / PK_L1BASIS.x; }
float3 SH_ToPrimeDir(const SH sh) { return sh.Y.wyz / (length(sh.Y.wyz) + 1e-6f); }

float3 SH_ToPrimeDir(const SH sh, inout float directionality) 
{
    float l = length(sh.Y.wyz);
    directionality = l / (sh.Y.x + 1e-6f);
    return sh.Y.wyz / l;
}

SH SH_FromRadiance(const float3 color, const float3 d)
{
	const float Co = color.r - color.b;
	const float T = color.b + Co * 0.5f;
	const float Cg = color.g - T;
	const float Y = max(T + Cg * 0.5f, 0.0);
	return SH(SH_GetBasis(d) * Y, float2(Co, Cg));
}
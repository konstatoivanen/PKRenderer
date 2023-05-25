#pragma once
#include Constants.glsl
#include Encoding.glsl

// Source: https://developer.download.nvidia.com/video/gputechconf/gtc/2019/presentation/s9985-exploring-ray-traced-future-in-metro-exodus.pdf
struct SH
{
	float4 Y;
	float2 CoCg;
};

#define pk_L1Basis float4(0.282095f, 0.488603f.xxx)
#define pk_L1Basis_Cosine float4(0.88622692545f, 1.02332670795.xxx)
#define pk_L1Basis_Irradiance float4(3.141593f, 2.094395f.xxx)
#define pk_ZeroSH SH(0.0f.xxxx, 0.0f.xx)

float4 SHGetBasis(const float3 d) { return float4(1.0f, d.yzx) * pk_L1Basis; }
float4 SHGetBasisCosine(const float3 d) { return float4(1.0f, d.yzx) * pk_L1Basis_Cosine; }
SH SHInterpolate(const SH sha, const SH shb, const float i) { return SH(lerp(sha.Y, shb.Y, i), lerp(sha.CoCg, shb.CoCg, i)); }
SH SHScale(const SH sh, float s) { return SH(sh.Y * s, sh.CoCg * s); }
SH SHAdd(const SH sha, const SH shb, float sb) { return SH(sha.Y + shb.Y * sb, sha.CoCg + shb.CoCg * sb); }

float3 SHToIrradiance(const SH sh, const float3 dir, const float chromaBias)
{
	float3 yCoCg;
	yCoCg.x = max(0.0f, 2.0 * (pk_L1Basis_Cosine.y * dot(sh.Y.wyz, dir) + pk_L1Basis_Cosine.x * sh.Y.x));
	yCoCg.yz = sh.CoCg * pk_L1Basis.x * yCoCg.x / (sh.Y.x + 1e-6);
	return PK_YCoCgRToRGB * (yCoCg * float3(1.0f - chromaBias, (1.0f + chromaBias).xx));
}

float3 SHToIrradiance(const SH sh, const float3 d) { return SHToIrradiance(sh, d, 1.0f); }
float3 SHToColor(const SH sh) { return PK_YCoCgRToRGB * float3(sh.Y.x / pk_L1Basis.x, sh.CoCg); }
float SHToLuminance(const SH sh, const float3 d) { return dot(pk_Luminance.xyz, SHToIrradiance(sh, d)); }
float SHToLuminance(const SH sh, const float3 d, const float chromaBias) { return dot(pk_Luminance.xyz, SHToIrradiance(sh, d, chromaBias)); }

SH SHFromRadiance(const float3 color, const float3 d)
{
	const float3 yCoCg = PK_RGBToYCoCgR * color;
	return SH(SHGetBasis(d) * yCoCg.x, yCoCg.yz);
}
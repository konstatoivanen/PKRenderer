#pragma once
#include Constants.glsl
#include Encoding.glsl

// Source: https://developer.download.nvidia.com/video/gputechconf/gtc/2019/presentation/s9985-exploring-ray-traced-future-in-metro-exodus.pdf
struct SH
{
	float4 Y;
	float2 CoCg;
};

#define pk_ZeroSH SH(0.0f.xxxx, 0.0f.xx)

float4 SH_GetBasis(const float3 d) { return float4(1.0f, d.yzx) * pk_L1Basis; }
float4 SH_GetBasisCosine(const float3 d) { return float4(1.0f, d.yzx) * pk_L1Basis_Cosine; }
SH SH_Interpolate(const SH sha, const SH shb, const float i) { return SH(lerp(sha.Y, shb.Y, i), lerp(sha.CoCg, shb.CoCg, i)); }
SH SH_Scale(const SH sh, float s) { return SH(sh.Y * s, sh.CoCg * s); }
SH SH_Add(const SH sha, const SH shb, float sb) { return SH(sha.Y + shb.Y * sb, sha.CoCg + shb.CoCg * sb); }

float3 SH_ToIrradiance(const SH sh, const float3 dir, const float chromaBias)
{
	float3 yCoCg;
	yCoCg.x = max(0.0f, 2.0 * (pk_L1Basis_Cosine.y * dot(sh.Y.wyz, dir) + pk_L1Basis_Cosine.x * sh.Y.x));
	yCoCg.yz = sh.CoCg * pk_L1Basis.x * yCoCg.x / (sh.Y.x + 1e-6);
	return PK_YCoCgRToRGB * (yCoCg * float3(1.0f - chromaBias, (1.0f + chromaBias).xx));
}

float3 SH_ToIrradiance(const SH sh, const float3 d) { return SH_ToIrradiance(sh, d, 1.0f); }
float3 SH_ToColor(const SH sh) { return PK_YCoCgRToRGB * float3(sh.Y.x / pk_L1Basis.x, sh.CoCg); }
float SH_ToLuminance(const SH sh, const float3 d) { return dot(pk_Luminance.xyz, SH_ToIrradiance(sh, d)); }
float SH_ToLuminance(const SH sh, const float3 d, const float chromaBias) { return dot(pk_Luminance.xyz, SH_ToIrradiance(sh, d, chromaBias)); }
float SH_ToLuminanceL0(const SH sh) { return sh.Y.x / pk_L1Basis.x; }
float3 SH_ToPrimeDir(const SH sh) { return sh.Y.wyz / (length(sh.Y.wyz) + 1e-6f); }

float3 SH_ToPrimeDir(const SH sh, inout float directionality) 
{
    float l = length(sh.Y.wyz);
    directionality = l / (sh.Y.x + 1e-6f);
    return sh.Y.wyz / l;
}

SH SH_FromRadiance(const float3 color, const float3 d)
{
	const float3 yCoCg = PK_RGBToYCoCgR * color;
	return SH(SH_GetBasis(d) * yCoCg.x, yCoCg.yz);
}
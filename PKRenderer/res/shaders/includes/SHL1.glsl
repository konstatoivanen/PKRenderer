#pragma once
#include Constants.glsl
#include Encoding.glsl

struct SH
{
	float4 Y;
	float2 CoCg;
};

#define pk_L1Basis float4(0.282095f, 0.488603f.xxx)
#define pk_L1Basis_Cosine float4(0.88622692545f, 1.02332670795.xxx)
#define pk_L1Basis_Irradiance float4(3.141593f, 2.094395f.xxx)
#define pk_ZeroSH SH(0.0f.xxxx, 0.0f.xx)

float4 GetSHBasis(const float3 d) { return float4(1.0f, d.yzx) * pk_L1Basis; }
float4 GetSHBasisCosine(const float3 d) { return float4(1.0f, d.yzx) * pk_L1Basis_Cosine; }

SH InterpolateSH(const SH sha, const SH shb, const float i) { return SH(lerp(sha.Y, shb.Y, i), lerp(sha.CoCg, shb.CoCg, i)); }
SH ScaleSH(const SH sh, float s) { return SH(sh.Y * s, sh.CoCg * s); }
SH AddSH(const SH sha, const SH shb, float sb) { return SH(sha.Y + shb.Y * sb, sha.CoCg + shb.CoCg * sb); }
float SHDot(float4 L1a, float4 L1b) { return max(0.0f, dot(L1a, L1b)); }

float3 SHToIrradiance(const SH sh, const float3 d) { return PK_YCoCgRToRGB * float3(SHDot(sh.Y, GetSHBasisCosine(d)), pk_L1Basis_Cosine.x * sh.CoCg);}
float3 SHToRadiance(const SH sh, const float3 d) { return PK_YCoCgRToRGB * float3(SHDot(sh.Y, GetSHBasis(d)), pk_L1Basis.x * sh.CoCg); }

float SHToLuminance(const SH sh, const float3 d) { return dot(pk_Luminance.xyz, SHToIrradiance(sh, d)); }
float SHToAmbientLuminance(const SH sh) { return dot(pk_Luminance.xyz, PK_YCoCgRToRGB * (float3(sh.Y.x, sh.CoCg) * pk_L1Basis_Cosine.x)); }

float3 SHToDominantDirection(const SH sh) { return normalize(sh.Y.wyz); }

SH IrradianceToSH(const float3 color, const float3 direction)
{
	const float3 yCoCg = PK_RGBToYCoCgR * color;
	return SH(GetSHBasis(direction) * yCoCg.x, pk_L1Basis.x * yCoCg.yz);
}
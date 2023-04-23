#pragma once
#include Constants.glsl
#include Encoding.glsl

struct SH
{
	float4 Y;
	float2 CoCg;
};

#define pk_SHL1B_IR float4(3.141593f, 2.094395f, 2.094395f, 2.094395f)
#define pk_SHL1B float4(0.282095f, 0.488603f, 0.488603f, 0.488603f)
#define pk_ZeroSH SH(0.0f.xxxx, 0.0f.xx)

float4 GetSHBasis(const float3 d) { return float4(1.0f, d.yzx) * pk_SHL1B; }
SH InterpolateSH(const SH sha, const SH shb, const float i) { return SH(lerp(sha.Y, shb.Y, i), lerp(sha.CoCg, shb.CoCg, i)); }
SH ScaleSH(const SH sh, float s) { return SH(sh.Y * s, sh.CoCg * s); }
SH AddSH(const SH sha, const SH shb, float sb) { return SH(sha.Y + shb.Y * sb, sha.CoCg + shb.CoCg * sb); }
float3 SHToIrradiance(const SH sh, const float3 d) { return PK_YCoCgRToRGB * float3(max(0.0f, dot(pk_SHL1B_IR * sh.Y, GetSHBasis(d))), pk_SHL1B_IR.x * sh.CoCg * pk_SHL1B.x);}
float3 SHToRadiance(const SH sh, const float3 d) { return PK_YCoCgRToRGB * float3(max(0.0f, dot(sh.Y, GetSHBasis(d))), pk_SHL1B.x * sh.CoCg); }
//float SHToLuminance(const SH sh, const float3 d) { return max(0.0f, dot(sh.Y, GetSHBasis(d))); }
float SHToLuminance(const SH sh, const float3 d) { return dot(pk_Luminance.xyz, SHToIrradiance(sh, d)); }
float SHToAmbientLuminance(const SH sh) { return dot(pk_Luminance.xyz, PK_YCoCgRToRGB * (float3(sh.Y.x, sh.CoCg) * pk_SHL1B_IR.x * pk_SHL1B.x)); }

SH IrradianceToSH(const float3 color, const float3 direction)
{
	const float3 yCoCg = PK_RGBToYCoCgR * color;
	return SH(GetSHBasis(direction) * yCoCg.x, pk_SHL1B.x * yCoCg.yz);
}
#pragma once
#include Constants.glsl
#include Encoding.glsl

struct SH
{
	float4 SHY;
	float2 CoCg;
};

#define pk_SHL1B_IR float4(3.141593f, 2.094395f, 2.094395f, 2.094395f)
#define pk_SHL1B float4(0.282095f, 0.488603f, 0.488603f, 0.488603f)

float4 GetSHBasis(const float3 d) { return float4(1.0f, d.yzx) * pk_SHL1B; }
SH InterpolateSH(const SH sha, const SH shb, const float i) { return SH(lerp(sha.SHY, shb.SHY, i), lerp(sha.CoCg, shb.CoCg, i)); }
SH ScaleSH(const SH sh, float s) { return SH(sh.SHY * s, sh.CoCg * s); }
SH AddSH(const SH sha, const SH shb, float sb) { return SH(sha.SHY + shb.SHY * sb, sha.CoCg + shb.CoCg * sb); }
float3 SHToIrradiance(const SH sh, const float3 d) { return PK_YCoCgRToRGB * float3(max(0.0f, dot(pk_SHL1B_IR * sh.SHY, GetSHBasis(d))), pk_SHL1B_IR.x * sh.CoCg * pk_SHL1B.x);}
float3 SHToRadiance(const SH sh, const float3 d) { return PK_YCoCgRToRGB * float3(max(0.0f, dot(sh.SHY, GetSHBasis(d))), pk_SHL1B.x * sh.CoCg); }
float SHToLuminance(const SH sh, const float3 d) { return max(0.0f, dot(sh.SHY, GetSHBasis(d))); }
float3 SHGetMaxRadiance(const SH sh) { return SHToRadiance(sh, normalize(sh.SHY.wyz)); }

SH IrradianceToSH(const float3 color, const float3 direction)
{
	const float3 yCoCg = PK_RGBToYCoCgR * color;
	return SH(GetSHBasis(direction) * yCoCg.x, pk_SHL1B.x * yCoCg.yz);
}

SH DegenerateSH(const SH sh, float normaldot)
{
	float4 Y = sh.SHY;
	Y.x += length(Y.yzw) * (1.0f - normaldot);
	Y.yzw *= normaldot;
	return SH(Y, sh.CoCg);
}
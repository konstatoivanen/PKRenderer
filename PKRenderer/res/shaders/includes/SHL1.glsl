#pragma once
#include Constants.glsl
#include Encoding.glsl

struct SH
{
	float4 SHY;
	float2 CoCg;
};

SH IrradianceToSH(const float3 color, const float3 direction)
{
	// Y00, Y1-1, Y10, Y11
	const float4 shBasisL1 = float4(0.282095f, 0.488603f * direction.y, 0.488603f * direction.z, 0.488603f * direction.x);
	const float shBasisL0 = 0.282095f;

	const float3 yCoCg = PK_RGBToYCoCgR * color;

	SH sh;
	sh.SHY = shBasisL1 * yCoCg.x;
	sh.CoCg = shBasisL0 * yCoCg.yz;
	return sh;
}

float3 SHToIrradiance(const SH sh, const float3 normal)
{
	const float Y = max(0.0f, 3.141593f * sh.SHY.x * 0.282095f + 
						2.094395f * sh.SHY.y * 0.488603f * normal.y + 
						2.094395f * sh.SHY.z * 0.488603f * normal.z + 
						2.094395f * sh.SHY.w * 0.488603f * normal.x);

	const float2 CoCg = 3.141593f * sh.CoCg * 0.282095f;

	return PK_YCoCgRToRGB * float3(Y, CoCg);
}

float3 SHToRadiance(const SH sh, const float3 direction)
{
	// Y00, Y1-1, Y10, Y11
	const float4 shBasisL1 = float4(0.282095f, 0.488603f * direction.y, 0.488603f * direction.z, 0.488603f * direction.x);
	const float shBasisL0 = 0.282095f;

	const float Y = max(0.0f, dot(sh.SHY, shBasisL1));
	const float2 CoCg = shBasisL0 * sh.CoCg;

	return PK_YCoCgRToRGB * float3(Y, CoCg);
}

SH InterpolateSH(const SH sha, const SH shb, const float i)
{
	SH shc;
	shc.SHY = lerp(sha.SHY, shb.SHY, i);
	shc.CoCg = lerp(sha.CoCg, shb.CoCg, i);
	return shc;
}

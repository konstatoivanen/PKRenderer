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
	const float4 shBasis = float4(0.282095f, 0.488603f * direction.y, 0.488603f * direction.z, 0.488603f * direction.x);
	const float3 yCoCg = RGBToYCoCg(color).xyz;

	SH sh;
	sh.SHY = shBasis * yCoCg.x;
	sh.CoCg = yCoCg.yz;
	return sh;
}

float3 SHToIrradiance(const SH sh, const float3 normal)
{
	float Y = max(0.0f, 3.141593f * sh.SHY.x * 0.282095f + 
						2.094395f * sh.SHY.y * 0.488603f * normal.y + 
						2.094395f * sh.SHY.z * 0.488603f * normal.z + 
						2.094395f * sh.SHY.w * 0.488603f * normal.x);

	return YCoCgToRGB(float3(Y, sh.CoCg));
}

float3 SHToRadiance(const SH sh, const float3 direction)
{
	// Y00, Y1-1, Y10, Y11
	const float4 shBasis = float4(0.282095f, 0.488603f * direction.y, 0.488603f * direction.z, 0.488603f * direction.x);
	float Y = max(0.0f, dot(sh.SHY, shBasis));
	float S = Y / (length(sh.SHY) + 1e-4f);
	return YCoCgToRGB(float3(Y, sh.CoCg * S));
}

SH InterpolateSH(const SH sha, const SH shb, const float i)
{
	SH shc;
	shc.SHY = lerp(sha.SHY, shb.SHY, i);
	shc.CoCg = lerp(sha.CoCg, shb.CoCg, i);
	return shc;
}

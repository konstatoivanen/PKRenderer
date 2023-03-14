#pragma once
#include Constants.glsl
#include Encoding.glsl

struct SH
{
	float4 SHY;
	float2 CoCg;
};

bool IsNaN(float x)
{
    return (floatBitsToUint(x) & 0x7fffffff) > 0x7f800000;
}

SH IrradianceToSH(const float3 color, const float3 direction)
{
	// Y00, Y1-1, Y10, Y11
	const float4 shBasis = float4(0.282095f, 0.488603f * direction.y, 0.488603f * direction.z, 0.488603f * direction.x);
	const float3 yCoCg = RGBToYCoCg(color);

	SH sh;
	sh.SHY = shBasis * yCoCg.x;
	sh.CoCg = yCoCg.yz;
	return sh;
}

float3 SHToIrradiance(const SH sh, const float3 normal)
{
	float Y = 3.141593f * sh.SHY.x * 0.282095f + 
			  2.094395f * sh.SHY.y * 0.488603f * normal.y + 
			  2.094395f * sh.SHY.z * 0.488603f * normal.z + 
			  2.094395f * sh.SHY.w * 0.488603f * normal.x;

	return YCoCgToRGB(float3(Y, sh.CoCg));
}

void AccumulateSH(inout SH sha, const SH shb, const uint s)
{
    if (IsNaN(sha.SHY.w))
    {
        sha = shb;
		return;
    }

	float istep = min(1.0f, PK_FOUR_PI / float(s));
	sha.SHY = lerp(sha.SHY, shb.SHY, istep);
	sha.CoCg = lerp(sha.CoCg, shb.CoCg, istep);
}
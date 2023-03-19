#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/Reconstruction.glsl
#include includes/SharedSceneGI.glsl

layout(rgba16f, set = PK_SET_SHADER) uniform image2DArray pk_ScreenGI_SHY_Write;
layout(rg16f, set = PK_SET_SHADER) uniform image2DArray pk_ScreenGI_CoCg_Write;
layout(r8ui, set = PK_SET_SHADER) uniform readonly restrict uimage2D pk_ScreenGI_Mask;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
	int2 size = imageSize(pk_ScreenGI_SHY_Write).xy;
	int2 coord = int2(gl_GlobalInvocationID.xy);

	if (Any_GEqual(coord, size))
	{
		return;
	}

	uint mask = imageLoad(pk_ScreenGI_Mask, coord).r;
	bool hasDiscontinuity = (mask & (1 << 0)) != 0;
	bool isActive = (mask & (1 << 1)) != 0;
	bool isOOB = (mask & (1 << 2)) != 0;

	if (isOOB)
	{
		return;
	}

	const float radius = hasDiscontinuity ? 1.0f : 0.25f;
	const float planeThreshold = hasDiscontinuity ? 0.5f : 0.25f;
	float theta = (pk_SceneGI_SampleIndex / pk_SceneGI_SampleCount) * PK_TWO_PI;
	float2 rotation = float2(cos(theta), sin(theta));

	// Find a base for the side cones with the normal as one of its base vectors.
	const float4 NR = SampleWorldSpaceNormalRoughness(coord);
	const float3 N = NR.xyz;
	const float3 U = abs(N.z) < 0.999f ? half3(0.0f, 0.0f, 1.0f) : half3(1.0f, 0.0f, 0.0f);
	const float3 T = normalize(cross(U, N));
	const float3 B = cross(N, T);
	const float3 O = SampleWorldPosition(coord, size);
	const float2 UV = (coord + 0.5f.xx) / size;

	float WDiff = 1.0f;
	float WSpec = 1.0f;
	float WSpecSample = unlerp_sat(0.2f, 0.8f, NR.w);

	SH irradiance;
	irradiance.SHY = tex2D(pk_ScreenGI_SHY_Read, float3(UV.xy, PK_GI_DIFF_LVL)).rgba;
	irradiance.CoCg = tex2D(pk_ScreenGI_CoCg_Read, float3(UV.xy, PK_GI_DIFF_LVL)).rg;

	SH radiance;
	radiance.SHY = tex2D(pk_ScreenGI_SHY_Read, float3(UV.xy, PK_GI_SPEC_LVL)).rgba;
	radiance.CoCg = tex2D(pk_ScreenGI_CoCg_Read, float3(UV.xy, PK_GI_SPEC_LVL)).rg;

	for (uint i = 0u; i < 22u; ++i)
	{
		float2 offset = PK_POISSON_DISK_22[i];
		offset *= length(offset);
		offset *= radius;

		offset.x = offset.x * rotation.x - offset.y * rotation.y;
		offset.y = offset.x * rotation.y + offset.y * rotation.x;

		float3 samplePos = O + B * offset.x + T * offset.y;
		float3 uvw;

		if (!TryGetWorldToClipUVW(samplePos, uvw))
		{
			continue;
		}

		samplePos = SampleWorldPosition(uvw.xy);
		float4 sampleNR = SampleWorldSpaceNormalRoughness(uvw.xy);

		const float3 sampleVec = samplePos - O;
		const float planeDist = abs(dot(sampleVec, N));
		const float sampleDist = dot(sampleVec, sampleVec);
		const float sampleAngle = dot(N, sampleNR.xyz);

		if (planeDist > planeThreshold || sampleDist > (radius * radius) || sampleAngle < 0.1f)
		{
			continue;
		}

		irradiance.SHY += tex2D(pk_ScreenGI_SHY_Read, float3(uvw.xy, PK_GI_DIFF_LVL)).rgba;
		irradiance.CoCg += tex2D(pk_ScreenGI_CoCg_Read, float3(uvw.xy, PK_GI_DIFF_LVL)).rg;

		radiance.SHY += tex2D(pk_ScreenGI_SHY_Read, float3(uvw.xy, PK_GI_SPEC_LVL)).rgba * WSpecSample;
		radiance.CoCg += tex2D(pk_ScreenGI_CoCg_Read, float3(uvw.xy, PK_GI_SPEC_LVL)).rg * WSpecSample;

		WSpec += WSpecSample;
		WDiff += 1.0f;
	}

	irradiance.SHY /= WDiff;
	irradiance.CoCg /= WDiff;

	radiance.SHY /= WSpec;
	radiance.CoCg /= WSpec;

	imageStore(pk_ScreenGI_SHY_Write, int3(coord, PK_GI_DIFF_LVL), irradiance.SHY);
	imageStore(pk_ScreenGI_CoCg_Write, int3(coord, PK_GI_DIFF_LVL), float4(irradiance.CoCg, 0.0f.xx));

	imageStore(pk_ScreenGI_SHY_Write, int3(coord, PK_GI_SPEC_LVL), radiance.SHY);
	imageStore(pk_ScreenGI_CoCg_Write, int3(coord, PK_GI_SPEC_LVL), float4(radiance.CoCg, 0.0f.xx));
}
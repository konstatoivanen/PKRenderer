#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

const float2 PK_POISSON_DISK_16_2[16] =
{
 float2(-0.94201624, -0.39906216),
 float2(0.94558609, -0.76890725),
 float2(-0.094184101, -0.92938870),
 float2(0.34495938, 0.29387760),
 float2(-0.91588581, 0.45771432),
 float2(-0.81544232, -0.87912464),
 float2(-0.38277543, 0.27676845),
 float2(0.97484398, 0.75648379),
 float2(0.44323325, -0.97511554),
 float2(0.53742981, -0.47373420),
 float2(-0.26496911, -0.41893023),
 float2(0.79197514, 0.19090188),
 float2(-0.24188840, 0.99706507),
 float2(-0.81409955, 0.91437590),
 float2(0.19984126, 0.78641367),
 float2(0.14383161, -0.14100790)
};

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;
void main()
{
	int2 size = imageSize(pk_ScreenGI_SHY_Write).xy;
	int2 coord = int2(gl_GlobalInvocationID.xy);

	if (Any_GEqual(coord, size))
	{
		return;
	}

	GIMask mask = LoadGIMask(coord);

	if (mask.isOOB)
	{
		return;
	}

	const float2 UV = (coord + 0.5f.xx) / size;
	const float D = SampleLinearDepth(UV);
	const float3 O = SampleWorldPosition(coord, size, D);
	const float4 NR = SampleWorldSpaceNormalRoughness(coord);
	const float3 N = NR.xyz;
	const float3x3 TBN = ComposeTBN(N);
	const float2 rotation = make_rotation((pk_SceneGI_SampleIndex / pk_SceneGI_SampleCount) * PK_TWO_PI);
	
	const float mindist = imageLoad(pk_ScreenGI_AO, coord).r;
	const float radius = mindist * lerp(0.1f, 3.0f, (D / pk_ProjectionParams.z)) * (1 + mask.discontinuityFrames);
	const float planedist = radius * 0.5f;

	float3 clipuvw;

	float2 W = 1.0f.xx;
	SH irradiance = ScaleSH(SampleGI_SH(UV, PK_GI_DIFF_LVL), W.x);
	SH radiance = ScaleSH(SampleGI_SH(UV, PK_GI_SPEC_LVL), W.y);
	
	for (uint i = 0u; i < 16u; ++i)
	{
		float2 offset = PK_POISSON_DISK_16_2[i] * length(PK_POISSON_DISK_16_2[i]) * radius;
		offset = float2(offset.x * rotation.x - offset.y * rotation.y, offset.x * rotation.y + offset.y * rotation.x);
	
		const float3 samplePos = O + TBN * float3(offset.xy, 0.0f);
		
		if (!TryGetWorldToClipUVW(samplePos, clipuvw))
		{
			continue;
		}
	
		const float3 sampleN = SampleWorldSpaceNormalRoughness(clipuvw.xy).xyz;
		const float3 sampleV = SampleWorldPosition(clipuvw.xy) - O;
		const float sampleDistY = abs(dot(sampleV, N));
		const float sampleDist = dot(sampleV, sampleV);
	
		if (sampleDistY > planedist || sampleDist > (radius * radius))
		{
			continue;
		}

		const float normalWeight = max(0.0f, dot(N, sampleN));
		const float planeWeight = max(0.0f, 1.0f - (sampleDistY / planedist));
		const float distWeight = max(0.0f, 1.0f - (sqrt(sampleDist) / radius));
		
		float2 WS;
		WS.x = normalWeight * planeWeight * distWeight;
		WS.y = WS.x * NR.w;

		irradiance = AddSH(irradiance, SampleGI_SH(clipuvw.xy, PK_GI_DIFF_LVL), WS.x);
		radiance = AddSH(radiance, SampleGI_SH(clipuvw.xy, PK_GI_SPEC_LVL), WS.y);
		W += WS;
	}

	irradiance.SHY /= W.x;
	irradiance.CoCg /= W.x;
	radiance.SHY /= W.y;
	radiance.CoCg /= W.y;
	StoreGI_SH(coord, PK_GI_DIFF_LVL, irradiance);
	StoreGI_SH(coord, PK_GI_SPEC_LVL, radiance);
}
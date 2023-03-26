#version 460
#pragma PROGRAM_COMPUTE
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

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

	float mindist = imageLoad(pk_ScreenGI_AO, coord).r;
	float2 extents;
	extents.x = max(mask.discontinuityFrames * 0.5f, pow2(mindist));
	extents.y = extents.x * 0.5f;

	const float2 rotation = make_rotation((pk_SceneGI_SampleIndex / pk_SceneGI_SampleCount) * PK_TWO_PI);

	const float3 O = SampleWorldPosition(coord, size);
	const float4 NR = SampleWorldSpaceNormalRoughness(coord);
	const float3 N = NR.xyz;
	const float4 NBasis = float4(0.282095f, 0.488603f * N.y, 0.488603f * N.z, 0.488603f * N.x);
	const float3x3 TBN = ComposeTBN(N);
	const float2 UV = (coord + 0.5f.xx) / size;
	float3 clipuvw;

	float2 W = 1.0f.xx;
	SH irradiance = ScaleSH(SampleGI_SH(UV, PK_GI_DIFF_LVL), W.x);
	SH radiance = ScaleSH(SampleGI_SH(UV, PK_GI_SPEC_LVL), W.y);

	for (uint i = 0u; i < 16u; ++i)
	{
		float2 offset = PK_POISSON_DISK_16[i] * length(PK_POISSON_DISK_16[i]) * extents.x;
		offset.x = offset.x * rotation.x - offset.y * rotation.y;
		offset.y = offset.x * rotation.y + offset.y * rotation.x;
	
		const float3 samplePos = O + TBN * float3(offset.xy, 0.0f);
		
		if (!TryGetWorldToClipUVW(samplePos, clipuvw))
		{
			continue;
		}
	
		const float3 sampleN = SampleWorldSpaceNormalRoughness(clipuvw.xy).xyz;
		const float3 sampleV = SampleWorldPosition(clipuvw.xy) - O;
		const float sampleDistY = abs(dot(sampleV, N));
		const float sampleDistXZ = dot(sampleV, sampleV);
		const float sampleDot = max(0.0f, dot(N, sampleN));
	
		if (sampleDistY < extents.y && sampleDistXZ < (extents.x * extents.x))
		{
			SH sampleSH = SampleGI_SH(clipuvw.xy, PK_GI_DIFF_LVL);
			float shd = saturate(dot(sampleSH.SHY.yzw, NBasis.yzw));
			shd = max(shd, mask.discontinuityFrames * 0.125f);

			float2 WS = float2(shd, sampleDot * NR.w * NR.w);
			irradiance = AddSH(irradiance, sampleSH, WS.x);
			radiance = AddSH(radiance, SampleGI_SH(clipuvw.xy, PK_GI_SPEC_LVL), WS.y);
			W += WS;
		}
	}

	irradiance.SHY /= W.x;
	irradiance.CoCg /= W.x;
	radiance.SHY /= W.y;
	radiance.CoCg /= W.y;
	StoreGI_SH(coord, PK_GI_DIFF_LVL, irradiance);
	StoreGI_SH(coord, PK_GI_SPEC_LVL, radiance);
}
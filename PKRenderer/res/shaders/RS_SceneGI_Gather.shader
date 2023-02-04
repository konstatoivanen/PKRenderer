#version 460
#include includes/Common.glsl
#include includes/Reconstruction.glsl
#include includes/SharedSceneGI.glsl

PK_DECLARE_LOCAL_CBUFFER(pk_RayGatherParams)
{
	uint pk_SampleIndex;
	uint pk_SampleCount;
};


struct TracePayload
{
	float hitDistance;
};

#pragma PROGRAM_RAY_GENERATION

PK_DECLARE_RT_PAYLOAD_OUT(TracePayload, payload, 0);

void main()
{
	int2 size = imageSize(pk_ScreenGI_Hits).xy;
	int2 coord = int2(gl_LaunchIDEXT.xy);

	if (Any_GEqual(coord, size))
	{
		return;
	}

	const float4 NR = SampleWorldSpaceNormalRoughness(coord);
	const float3 N = NR.xyz;
	const float3 O = SampleWorldPosition(coord, size);
	const float3 V = normalize(O - pk_WorldSpaceCameraPos.xyz);
	const float3 R = GlobalNoiseBlue(uint2(coord)).xyz;
	
	// const float3 D = GetSampleDirectionSE(N, pk_SampleIndex, pk_SampleCount, R.x);
	const float3 D = GetSampleDirectionHammersLey(pk_SampleIndex, pk_SampleCount, R.xy, N);

	payload.hitDistance = PK_GI_RAY_MAX_DISTANCE;
	traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, O, PK_GI_RAY_MIN_DISTANCE, D, PK_GI_RAY_MAX_DISTANCE, 0);
	imageStore(pk_ScreenGI_Hits, int3(coord, pk_SampleIndex), float4(payload.hitDistance, 0.0f.xxx));
}

#pragma PROGRAM_RAY_MISS
PK_DECLARE_RT_PAYLOAD_IN(TracePayload, payload, 0);

void main()
{
	// Do nothing for now
}

#pragma PROGRAM_RAY_CLOSEST_HIT
PK_DECLARE_RT_PAYLOAD_IN(TracePayload, payload, 0);

void main()
{
	payload.hitDistance = PK_GET_RAY_HIT_DISTANCE;
}

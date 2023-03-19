#version 460
#include includes/Common.glsl
#include includes/Reconstruction.glsl
#include includes/SharedSceneGI.glsl

struct TracePayload
{
	float hitDistance;
};

#pragma PROGRAM_RAY_GENERATION

layout(rg16f, set = PK_SET_SHADER) uniform image2D pk_ScreenGI_Hits;

PK_DECLARE_RT_PAYLOAD_OUT(TracePayload, payload, 0);

void main()
{
	int2 size = imageSize(pk_ScreenGI_Hits).xy;
	int2 coord = int2(gl_LaunchIDEXT.xy);

	const float4 NR = SampleWorldSpaceNormalRoughness(coord);
	const float3 N = NR.xyz;
	const float3 O = SampleWorldPosition(coord, size) + N * PK_GI_RAY_MIN_DISTANCE;
	const float3 V = normalize(O - pk_WorldSpaceCameraPos.xyz);
	const float3 R = GlobalNoiseBlue(uint2(coord) + pk_FrameIndex.xx).xyz;
	const float3 D0 = ImportanceSampleGGX(pk_SceneGI_SampleIndex, pk_SceneGI_SampleCount, N, 1.0f, R.xy);
	const float3 D1 = ImportanceSampleGGX(pk_SceneGI_SampleIndex, pk_SceneGI_SampleCount, N, V, NR.w, R.xy);

	float distanceD, distanceR;

	payload.hitDistance = PK_GI_RAY_MAX_DISTANCE;
	traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, O, PK_GI_RAY_MIN_DISTANCE, D0, PK_GI_RAY_MAX_DISTANCE, 0);
	distanceD = payload.hitDistance;

	payload.hitDistance = PK_GI_RAY_MAX_DISTANCE;
	traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, O, PK_GI_RAY_MIN_DISTANCE, D1, PK_GI_RAY_MAX_DISTANCE, 0);
	distanceR = payload.hitDistance;

	imageStore(pk_ScreenGI_Hits, coord, float4(distanceD, distanceR, 0.0f.xx));
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

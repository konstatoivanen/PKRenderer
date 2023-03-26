#version 460
#include includes/Common.glsl
#include includes/SharedSceneGI.glsl
#include includes/Reconstruction.glsl

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

	float depth = SampleLinearDepth(coord);
	depth -= depth * 1e-2f; // Apply Bias to avoid rays clipping with geo

	const float4 NR = SampleWorldSpaceNormalRoughness(coord);
	const float3 N = NR.xyz;
	// @TODO HACK: Due to normal mapping the ray could intersect immediately, causing a feedback loop.
	// Offsetting origin by min distance fixes this, but causes incorrect near hits.
	const float3 O = SampleWorldPosition(coord, size, depth) + N * PK_GI_RAY_MIN_DISTANCE;
	const float3 V = normalize(O - pk_WorldSpaceCameraPos.xyz);
	const float2 Xi = GetSampleOffset(GlobalNoiseBlue(coord + pk_FrameIndex / PK_GI_SAMPLE_COUNT).xy);
	const float3 D0 = ImportanceSampleGGX(Xi, N, 1.0f);
	const float3 D1 = ImportanceSampleGGX(Xi, N, V, NR.w);

	float distanceD, distanceR;

	payload.hitDistance = PK_GI_RAY_MAX_DISTANCE;
	traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, O, 0.0f, D0, PK_GI_RAY_MAX_DISTANCE, 0);
	distanceD = payload.hitDistance;

	payload.hitDistance = PK_GI_RAY_MAX_DISTANCE;
	traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, O, 0.0f, D1, PK_GI_RAY_MAX_DISTANCE, 0);
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

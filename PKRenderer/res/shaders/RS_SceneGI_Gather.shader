#version 460
#include includes/Common.glsl
#include includes/Reconstruction.glsl
#include includes/SharedSceneGI.glsl

//struct TracePayload
//{
//	float4 outColor;
//	float3 outPosition;
//	float inConeSize;
//};

struct TracePayload
{
	float hitDistance;
};

#pragma PROGRAM_RAY_GENERATION

layout(r8ui, set = PK_SET_SHADER) uniform readonly restrict uimage2D pk_ScreenGI_Mask;

PK_DECLARE_RT_PAYLOAD_OUT(TracePayload, payload, 0);

/*
float4 RayTraceDiffuse(float3 origin, const float3 normal, const float dither)
{
	const float tmin = 0.001;
	const float tmax = 10000.0;
	
	float4 A = 0.0.xxxx;

	#pragma unroll 16
	for (uint i = 0u; i < 16u; ++i)
	{
		const float3 direction = GetSampleDirectionSE(normal, i, 16u, dither);

		payload.outColor = 0.0f.xxxx;
		payload.outPosition = 0.0f.xxx;
		payload.inConeSize = 1.0f;

		traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, tmin, direction, tmax, 0);
		A += payload.outColor * max(0.0, dot(normal, direction));
	}

	A /= 16.0f;
	A.a = 1.0f - A.a;

	A.a *= max(0.0, dot(normal, float3(0.0, 1.0, 0.0)) * 0.5 + 0.5);
	A.rgb *= PK_GI_DIFFUSE_GAIN;

	return A;
}

*/
float4 RayTraceSpecular(float3 origin, const float3 normal, const float3 direction, const float roughness)
{
	const float tmin = 0.01;
	const float tmax = 100.0;

	//payload.outColor = 0.0f.xxxx;
	//payload.outPosition = 0.0f.xxx;
	//payload.inConeSize = pow2(roughness) * 2.5f;
	float coneSize = pow2(roughness) * 2.5f;

	payload.hitDistance = tmax;
	traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, tmin, direction, tmax, 0);
	
	payload.hitDistance = max(PK_GI_VOXEL_SIZE, payload.hitDistance);

	float level = (payload.hitDistance / PK_GI_VOXEL_SIZE) * coneSize;
	//
	//payload.outPosition = PK_GET_RAY_HIT_POINT;
	//payload.outColor = SampleSceneGI(payload.outPosition, level);
	float4 outColor = SampleSceneGI(origin + direction * payload.hitDistance, level);

	outColor.rgb *= PK_GI_SPECULAR_GAIN;
	outColor.a = (1.0f - outColor.a) / max(1.0f, level);

	return outColor;
}

float4 RayTraceSpecular2(float3 origin, const float3 normal, const float3 direction, float roughness)
{
	const float tmin = 0.01;
	const float tmax = 100.0;

	float4 color = float4(0.0f.xxx, 1.0f);
	float conesize = pow2(roughness) * 2.5f;

	payload.hitDistance = tmax;
	traceRayEXT(pk_SceneStructure, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin, tmin, direction, tmax, 0);

	float dist = min(payload.hitDistance, (1.0f + PK_INV_SQRT2) * PK_GI_VOXEL_SIZE);
	float alphaMult = dist / ((1.0f + PK_INV_SQRT2) * PK_GI_VOXEL_SIZE);

	uint iterations = clamp(uint(-(8.0f * (PK_GI_VOXEL_SIZE - payload.hitDistance)) / (PK_GI_VOXEL_SIZE * conesize)), 1u, 64u);

	for (uint i = 0; i < iterations; ++i)
	{
		float level = i * conesize;

		float4 voxel = SampleSceneGI(origin + dist * direction, level);

		color.rgb += voxel.rgb * voxel.a * color.a * (1.0 + level);
		color.a *= saturate(1.0 - voxel.a * (1.0 + pow3(level) * conesize * 0.02));
		// simpler version: color.a *= 1.0f - voxel.a;

		dist += PK_GI_VOXEL_SIZE * (1.0f + 0.125f * level);
	}

	color.rgb *= PK_GI_SPECULAR_GAIN;

	color.a *= alphaMult * alphaMult;

	return color;
}

void main()
{
	int2 size = imageSize(pk_ScreenGI_Write).xy;
	int2 coord = int2(gl_LaunchIDEXT.xy);

	if (Any_GEqual(coord, size))
	{
		return;
	}

	uint mask = imageLoad(pk_ScreenGI_Mask, coord).r;
	bool hasDiscontinuity = (mask & (1 << 0)) != 0;
	bool isActive = (mask & (1 << 1)) != 0;
	bool isOOB = (mask & (1 << 2)) != 0;

	float3 worldposition = SampleWorldPosition(coord, size);

	if (!isOOB && (hasDiscontinuity || isActive))
	{
		// Find a base for the side cones with the normal as one of its base vectors.
		const float4 NR = SampleWorldSpaceNormalRoughness(coord);
		const float3 N = NR.xyz;
		const float3 O = worldposition;
		const float3 V = normalize(worldposition - pk_WorldSpaceCameraPos.xyz);
		const float3 R = reflect(V, N);
		const float3 D = GlobalNoiseBlue(uint2(coord + pk_Time.xy * 512)).xyz;

		imageStore(pk_ScreenGI_Write, int3(coord, PK_GI_DIFF_LVL), ConeTraceDiffuse(O, N, D.x));
		imageStore(pk_ScreenGI_Write, int3(coord, PK_GI_SPEC_LVL), RayTraceSpecular2(O, N, R, NR.w));
	}
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
	//float dist = PK_GET_RAY_HIT_DISTANCE;
	//float level = (dist / PK_GI_VOXEL_SIZE) * payload.inConeSize;
	//
	//payload.outPosition = PK_GET_RAY_HIT_POINT;
	//payload.outColor = SampleSceneGI(payload.outPosition, level);
}

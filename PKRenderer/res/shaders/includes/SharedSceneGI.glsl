#pragma once
#include Utilities.glsl

PK_DECLARE_CBUFFER(pk_SceneGI_Params, PK_SET_SHADER)
{
	float4 pk_SceneGI_ST;
	uint4 pk_SceneGI_Swizzle;
	int4 pk_SceneGI_Checkerboard_Offset;
	float pk_SceneGI_VoxelSize; 
	float pk_SceneGI_ConeAngle; 
	float pk_SceneGI_DiffuseGain; 
	float pk_SceneGI_SpecularGain; 
	float pk_SceneGI_Fade; 
};

layout(r8ui, set = PK_SET_SHADER) uniform uimage3D pk_SceneGI_VolumeMaskWrite;
layout(rgba16, set = PK_SET_SHADER) uniform image3D pk_SceneGI_VolumeWrite;
layout(rgba16, set = PK_SET_SHADER) uniform image2DArray pk_ScreenGI_Write;
layout(r16f, set = PK_SET_SHADER) uniform image2DArray pk_ScreenGI_Hits;
PK_DECLARE_SET_SHADER uniform sampler3D pk_SceneGI_VolumeRead;

#define PK_GI_DIFF_LVL 0
#define PK_GI_SPEC_LVL 1
#define PK_GI_VOXEL_SIZE pk_SceneGI_VoxelSize
#define PK_GI_ANGLE pk_SceneGI_ConeAngle
#define PK_GI_DIFFUSE_GAIN pk_SceneGI_DiffuseGain.xxx
#define PK_GI_SPECULAR_GAIN pk_SceneGI_SpecularGain.xxx
#define PK_GI_CHECKERBOARD_OFFSET pk_SceneGI_Checkerboard_Offset.xy
#define PK_GI_RAY_MIN_DISTANCE 0.01f
#define PK_GI_RAY_MAX_DISTANCE 50.0f
#define PK_GI_RAY_COUNT 16u

float3 GetSampleDirectionSE(float3 worldNormal, uint index, const float sampleCount, float dither)
{
    float fi = float(index) + dither;
	float fiN = fi / sampleCount;
	float longitude = PK_GI_ANGLE * fi;
	float latitude = asin(fiN * 2.0 - 1.0);

	float3 kernel;
	kernel.x = cos(latitude) * cos(longitude);
	kernel.z = cos(latitude) * sin(longitude);
	kernel.y = sin(latitude);
	kernel = faceforward(kernel, kernel, -worldNormal); 

	return normalize(kernel);
}

float RadicalInverse_VdC(uint bits)													
{																						
    bits = (bits << 16u) | (bits >> 16u);												
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);				
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);				
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);				
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);				
    return float(bits) * 2.3283064365386963e-10;										
}																						
																						
float2 Hammersley(uint i, uint N)														
{					
    return float2(float(i % N) / float(N), RadicalInverse_VdC(i));							
}																						
																						
float3 GetSampleDirectionHammersLey(uint i, uint s, float2 d, float3 N)								
{															
	float2 Xi = Hammersley(i,s);
	Xi += d;
	Xi -= floor(Xi);

    float phi = 2.0 * 3.14159265 * Xi.x;												
    float cosTheta = sqrt((1.0 - Xi.y) / 1.0);				
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);									
																						
    vec3 H;																			
    H.x = cos(phi) * sinTheta;															
    H.y = sin(phi) * sinTheta;															
    H.z = cosTheta;																	
																						
    float3 up = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);			
    float3 tangent = normalize(cross(up, N));											
    float3 bitangent = cross(N, tangent);												
    float3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;						
    return normalize(sampleVec);														
}																						

float3 VoxelToWorldSpace(int3 coord) { return (float3(coord) * PK_GI_VOXEL_SIZE) + pk_SceneGI_ST.xyz + PK_GI_VOXEL_SIZE * 0.5f; }

int3 WorldToVoxelSpace(float3 worldposition) { return int3((worldposition - pk_SceneGI_ST.xyz) * pk_SceneGI_ST.www); }

float3 WorldToSampleSpace(float3 worldposition) { return ((worldposition - pk_SceneGI_ST.xyz) * pk_SceneGI_ST.www) / textureSize(pk_SceneGI_VolumeRead, 0).xyz;  }

float3 WorldToVoxelClipSpace(float3 worldposition) { return WorldToSampleSpace(worldposition) * 2.0f - 1.0f; }

float4 WorldToVoxelNDCSpace(float3 worldposition) 
{ 
	float3 clippos = WorldToVoxelClipSpace(worldposition);
	return float4(clippos[pk_SceneGI_Swizzle.x], clippos[pk_SceneGI_Swizzle.y], clippos[pk_SceneGI_Swizzle.z] * 0.5f + 0.5f, 1);
}

bool SceneGIVoxelHasValue(float3 worldposition)
{
	int3 coord = WorldToVoxelSpace(worldposition);
	return imageLoad(pk_SceneGI_VolumeMaskWrite, coord).x != 0;
}

float4 SampleSceneGI(float3 worldposition, float level)
{
    float4 value = tex2DLod(pk_SceneGI_VolumeRead, WorldToSampleSpace(worldposition), level);
    value.rgb *= 128.0f;
    return value;
}

void StoreSceneGI(float3 worldposition, float4 color) 
{ 
	int3 coord = WorldToVoxelSpace(worldposition);
	float4 value = float4(color.rgb / 128.0f, color.a);
	
	// Quantize colors down so that we don't get any lingering artifacts from dim light sources.
	value.rgb = floor(value.rgb * 0xFFFF.xxx) / 0xFFFF.xxx;

	imageStore(pk_SceneGI_VolumeMaskWrite, coord, uint4(1u));
	imageStore(pk_SceneGI_VolumeWrite, coord, value); 
}


float4 SampleGIVolumetric(float3 position)
{
	float4 color = float4(0.0.xxx, 1.0);

	for (uint i = 0; i < 4; ++i)
	{
		float level = i * 1.25f;

		float4 voxel = SampleSceneGI(position, level);

		color.rgb += voxel.rgb * (1.0 + level) * pow2(color.a) * i;
		color.a *= saturate(1.0 - voxel.a * (1.0 + pow3(level) * 0.075));
	}

	return color;
}

float4 ConeTraceDiffuse(float3 origin, const float3 normal, const float dither) 
{
	float4 A = 0.0.xxxx;

	origin += normal * (1.0 + PK_INV_SQRT2) * PK_GI_VOXEL_SIZE;

	#pragma unroll 16
	for (uint i = 0u; i < 16u; ++i)
	{
		const float3 direction = GetSampleDirectionSE(normal, i, 16u, dither);

		float4 color = float4(0.0.xxx, 1.0);

		float dist = 1.0f;

		#pragma unroll 4
		for (uint i = 0; i < 4; ++i)
		{
			float level = log2(dist / PK_GI_VOXEL_SIZE);
			float4 voxel = SampleSceneGI(origin + dist * direction, level);

			color.rgb += voxel.rgb * (1.0f + level) * pow2(color.a) * i;
			color.a *= saturate(1.0f - voxel.a * (1.0f + pow3(level) * 0.1f));

			dist += pow2(level + 1.0f) * PK_GI_VOXEL_SIZE;
		}

		A += color * max(0.0, dot(normal, direction));
	}

	float groundOcclusion = saturate(normal.y + 1.0f);

	A.a *= groundOcclusion;
	A.rgb *= PK_GI_DIFFUSE_GAIN;

	A /= 16.0;

	return A;
}

float4 ConeTraceSpecular(float3 origin, const float3 normal, const float3 direction, float roughness) 
{
	origin += normal * (1.0f + PK_INV_SQRT2) * PK_GI_VOXEL_SIZE;

	float4 color = float4(0.0f.xxx, 1.0f);

	float dist = 0.0f;

	float conesize = pow2(roughness) * 2.5f;

	for (uint i = 0; i < 64; ++i)
	{
		float level = i * conesize;

		float4 voxel = SampleSceneGI(origin + dist * direction, level);

		color.rgb += voxel.rgb * voxel.a * color.a * (1.0 + level);
		color.a *= saturate(1.0 - voxel.a * (1.0 + pow3(level) * conesize * 0.02)); 
		// simpler version: color.a *= 1.0f - voxel.a;

		dist += PK_GI_VOXEL_SIZE * (1.0f + 0.125f * level);
	}

	color.rgb *= PK_GI_SPECULAR_GAIN;

	return color;
}

float4 GatherRayHits(uint2 coord, float3 origin, float3 normal, float2 dither)
{
	float4 A = 0.0f.xxxx;

	for (uint i = 0u; i < PK_GI_RAY_COUNT; ++i)
	{
		const float3 direction = GetSampleDirectionHammersLey(i, PK_GI_RAY_COUNT, dither, normal);
		//const float3 direction = GetSampleDirectionSE(normal, i, PK_GI_RAY_COUNT, dither);
		
		const float sampleDistance = imageLoad(pk_ScreenGI_Hits, int3(coord, i)).r;
		const float3 samplePosition = origin +  direction * sampleDistance;
		const float level = 0.5f * sqrt(sampleDistance / PK_GI_VOXEL_SIZE);
		const float ndotl = max(0.0, dot(direction, normal));
		const float4 voxel = SampleSceneGI(samplePosition, level);
		
		A.rgb += voxel.rgb * ndotl * pow3(1.0f + level);
		A.a += voxel.a * (1.0f + pow3(level));
	}

	A /= PK_GI_RAY_COUNT;
	A.a = saturate(1.0f - A.a);

	return A;
}
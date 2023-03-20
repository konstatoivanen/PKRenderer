#pragma once
#include Utilities.glsl
#include SampleDistribution.glsl
#include SHL1.glsl

PK_DECLARE_CBUFFER(pk_SceneGI_Params, PK_SET_SHADER)
{
	float4 pk_SceneGI_ST;
	uint4 pk_SceneGI_Swizzle;
	int4 pk_SceneGI_Checkerboard_Offset;
	uint pk_SceneGI_SampleIndex;
	uint pk_SceneGI_SampleCount;
	float pk_SceneGI_VoxelSize; 
	float pk_SceneGI_LuminanceGain; 
	float pk_SceneGI_ChrominanceGain; 
};

layout(r8ui, set = PK_SET_SHADER) uniform uimage3D pk_SceneGI_VolumeMaskWrite;
layout(rgba16, set = PK_SET_SHADER) uniform image3D pk_SceneGI_VolumeWrite;
PK_DECLARE_SET_SHADER uniform sampler3D pk_SceneGI_VolumeRead;
PK_DECLARE_SET_SHADER uniform sampler2DArray pk_ScreenGI_SHY_Read;
PK_DECLARE_SET_SHADER uniform sampler2DArray pk_ScreenGI_CoCg_Read;

#define PK_GI_DIFF_LVL 0
#define PK_GI_SPEC_LVL 1
#define PK_GI_VOXEL_MAX_MIP 7
#define PK_GI_VOXEL_SIZE pk_SceneGI_VoxelSize
#define PK_GI_VOXEL_LENGTH pk_SceneGI_VoxelSize * PK_SQRT2
#define PK_GI_CHECKERBOARD_OFFSET pk_SceneGI_Checkerboard_Offset.xy
#define PK_GI_RAY_MIN_DISTANCE 0.01f
#define PK_GI_RAY_MAX_DISTANCE 100.0f
#define PK_GI_HDR_FACTOR 128.0f

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

float4 SampleGI_WS(float3 worldposition, float level)
{
    float4 value = tex2DLod(pk_SceneGI_VolumeRead, WorldToSampleSpace(worldposition), level);
    value.rgb *= PK_GI_HDR_FACTOR;
    return value;
}

void StoreGI_WS(float3 worldposition, float4 color) 
{ 
	int3 coord = WorldToVoxelSpace(worldposition);
	float4 value = float4(color.rgb / PK_GI_HDR_FACTOR, color.a);
	
	// Quantize colors down so that we don't get any lingering artifacts from dim light sources.
	value.rgb = floor(value.rgb * 0xFFFF.xxx) / 0xFFFF.xxx;

	imageStore(pk_SceneGI_VolumeMaskWrite, coord, uint4(1u));
	imageStore(pk_SceneGI_VolumeWrite, coord, value); 
}

float3 SampleGI_VS_Diffuse(const float2 uv, const float3 N)
{
	SH irradianceSH;
	irradianceSH.SHY = tex2D(pk_ScreenGI_SHY_Read, float3(uv, PK_GI_DIFF_LVL)).rgba;
    irradianceSH.CoCg = tex2D(pk_ScreenGI_CoCg_Read, float3(uv, PK_GI_DIFF_LVL)).rg;
	irradianceSH.SHY *= pk_SceneGI_LuminanceGain;
	irradianceSH.CoCg *= pk_SceneGI_ChrominanceGain;
	return SHToIrradiance(irradianceSH, N);
}

void SampleGI_VS(inout float3 diffuse, inout float3 specular, const float2 uv, const float3 N, const float3 V, const float R)
{
	SH irradianceSH;
	irradianceSH.SHY = tex2D(pk_ScreenGI_SHY_Read, float3(uv, PK_GI_DIFF_LVL)).rgba;
    irradianceSH.CoCg = tex2D(pk_ScreenGI_CoCg_Read, float3(uv, PK_GI_DIFF_LVL)).rg;
    
	SH radianceSH;
	radianceSH.SHY = tex2D(pk_ScreenGI_SHY_Read, float3(uv, PK_GI_SPEC_LVL)).rgba;
    radianceSH.CoCg = tex2D(pk_ScreenGI_CoCg_Read, float3(uv, PK_GI_SPEC_LVL)).rg;

	irradianceSH.SHY *= pk_SceneGI_LuminanceGain;
	irradianceSH.CoCg *= pk_SceneGI_ChrominanceGain;
	radianceSH.SHY *= pk_SceneGI_LuminanceGain;
	radianceSH.CoCg *= pk_SceneGI_ChrominanceGain;

    diffuse = SHToIrradiance(irradianceSH, N);
    specular = SHToRadiance(radianceSH, normalize(reflect(V, N))) / sqrt(R);
}

float4 SampleGIVolumetric(float3 position)
{
	float4 color = float4(0.0.xxx, 1.0);

	for (uint i = 0; i < 4; ++i)
	{
		float level = i * 1.25f;

		float4 voxel = SampleGI_WS(position, level);

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
		const float3 direction = GetSampleDirectionSE(normal, i, 16u, dither, 5.08320368996f);

		float4 color = float4(0.0.xxx, 1.0);

		float dist = 1.0f;

		#pragma unroll 4
		for (uint i = 0; i < 4; ++i)
		{
			float level = log2(dist / PK_GI_VOXEL_SIZE);
			float4 voxel = SampleGI_WS(origin + dist * direction, level);

			color.rgb += voxel.rgb * (1.0f + level) * pow2(color.a) * i;
			color.a *= saturate(1.0f - voxel.a * (1.0f + pow3(level) * 0.1f));

			dist += pow2(level + 1.0f) * PK_GI_VOXEL_SIZE;
		}

		A += color * max(0.0, dot(normal, direction));
	}

	float groundOcclusion = saturate(normal.y + 1.0f);

	A.a *= groundOcclusion;
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

		float4 voxel = SampleGI_WS(origin + dist * direction, level);

		color.rgb += voxel.rgb * voxel.a * color.a * (1.0 + level);
		color.a *= saturate(1.0 - voxel.a * (1.0 + pow3(level) * conesize * 0.02)); 
		// simpler version: color.a *= 1.0f - voxel.a;

		dist += PK_GI_VOXEL_SIZE * (1.0f + 0.125f * level);
	}

	return color;
}

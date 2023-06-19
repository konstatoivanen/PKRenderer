#pragma once
#include Common.glsl
#include Noise.glsl
#include BlueNoise.glsl

#define VOLUME_CELL_SIZE 8u
#define VOLUME_DEPTH 128
#define VOLUME_SIZE_XY uint2(pk_ScreenSize.xy / VOLUME_CELL_SIZE)

#define VOLUME_INV_DEPTH 0.0078125f // 1.0f / 128.0f
#define VOLUME_COMPOSITE_DITHER_AMOUNT 2.0f * float3(1.0f.xx / VOLUME_SIZE_XY, VOLUME_INV_DEPTH)
#define VOLUME_MIN_DENSITY 0.000001f
#define VOLUME_ACCUMULATION clamp(20.0f * pk_DeltaTime.x, 0.01f, 1.0f)

PK_DECLARE_CBUFFER(pk_VolumeResources, PK_SET_SHADER)
{
    float4 pk_Volume_WindDir;
    float pk_Volume_ConstantFog;
    float pk_Volume_HeightFogExponent;
    float pk_Volume_HeightFogOffset;
    float pk_Volume_HeightFogAmount;
    float pk_Volume_Density;
    float pk_Volume_Intensity;
    float pk_Volume_Anisotropy;
    float pk_Volume_NoiseFogAmount;
    float pk_Volume_NoiseFogScale;
    float pk_Volume_WindSpeed;
};

PK_DECLARE_SET_SHADER uniform sampler3D pk_Volume_ScatterRead;
PK_DECLARE_SET_SHADER uniform sampler3D pk_Volume_InjectRead;
layout(rgba16f, set = PK_SET_SHADER) uniform image3D pk_Volume_Inject;
layout(rgba16f, set = PK_SET_SHADER) uniform image3D pk_Volume_Scatter;

float GetVolumeCellDepth(float index)
{
    return pk_ProjectionParams.x * pow(pk_ExpProjectionParams.z, index / VOLUME_DEPTH);
}

float2 GetVolumeCellDepth(float2 index)
{
    return pk_ProjectionParams.xx * pow(pk_ExpProjectionParams.zz, index * VOLUME_INV_DEPTH);
}

float GetVolumeWCoord(float depth)
{
    return max(log2(depth) * pk_ExpProjectionParams.x + pk_ExpProjectionParams.y, 0.0);
}

float3 GetVolumeCellNoise(uint3 id)
{
    return GlobalNoiseBlue(id.xy + pk_FrameIndex.x, pk_FrameIndex.x);
}

float3 ReprojectWorldToCoord(float3 worldpos)
{
    float3 uvw = ClipToUVW(mul(pk_MATRIX_L_VP, float4(worldpos, 1.0f)));
    uvw.z = GetVolumeWCoord(LinearizeDepth(uvw.z));
    return uvw;
}

float3 ReprojectViewToCoord(float3 viewpos)
{
	float3 uvw = ClipToUVW(mul(pk_MATRIX_LD_P, float4(viewpos, 1.0f)));
    uvw.z = GetVolumeWCoord(LinearizeDepth(uvw.z));
    return uvw;
}
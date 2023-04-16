#pragma once
#include Common.glsl
#include Noise.glsl
#include BlueNoise.glsl

#define VOLUME_DEPTH 128
#define VOLUME_INV_DEPTH 0.0078125f // 1.0f / 128.0f
#define VOLUME_WIDTH 160
#define VOLUME_HEIGHT 90
#define VOLUME_SIZE_ST float3(0.00625f, 0.0111111111111111f, 0.5f) // (1.0f / 160.0f, 1.0f / 90.0f, 0.5f)
#define VOLUME_COMPOSITE_DITHER_AMOUNT 2.0f * float3(0.00625f, 0.0111111111111111f, 0.0078125f)
#define VOLUME_DEPTH_BATCH_SIZE_PX 16
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

PK_DECLARE_BUFFER(uint, pk_VolumeMaxDepths, PK_SET_SHADER);

#define VOLUME_LOAD_MAX_DEPTH(index) uintBitsToFloat(PK_BUFFER_DATA(pk_VolumeMaxDepths, index))

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
    return GlobalNoiseBlue(id.xy + id.z * int2(VOLUME_WIDTH, VOLUME_HEIGHT) + int(pk_Time.w * 1000).xx);
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

uint GetVolumeDepthTileIndex(float2 uv)
{
    return uint(uv.x * VOLUME_WIDTH) + VOLUME_WIDTH * uint(uv.y * VOLUME_HEIGHT);
}
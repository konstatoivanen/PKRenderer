#pragma once
#include GBuffers.glsl
#include Noise.glsl
#include BlueNoise.glsl
#include TricubicSampler.glsl

#define VOLUME_CELL_SIZE 8u
#define VOLUME_DEPTH 128
#define VOLUME_SIZE_XY uint2(pk_ScreenSize.xy / VOLUME_CELL_SIZE)
#define VOLUME_SIZE uint3(pk_ScreenSize.xy / VOLUME_CELL_SIZE, 128)

#define VOLUME_INV_DEPTH 0.0078125f // 1.0f / 128.0f
#define VOLUME_COMPOSITE_DITHER_AMOUNT float2(1.0f.xx / VOLUME_SIZE_XY)
#define VOLUME_MIN_DENSITY 1e-5f
#define VOLUME_ACCUMULATION 0.03f//clamp(10.0f * pk_DeltaTime.x, 0.01f, 1.0f)

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

DEFINE_TRICUBIC_SAMPLER(pk_Volume_ScatterRead, VOLUME_SIZE)

DEFINE_TRICUBIC_SAMPLER(pk_Volume_InjectRead, VOLUME_SIZE)

float GetVolumeAccumulation(float3 uvw)
{
    return pk_FrameIndex.y == 0u || uvw.z <= 0.0f ? 1.0f : VOLUME_ACCUMULATION;;
}

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

float3 GetVolumeCellDither(uint2 coord)
{
    float3 n = GlobalNoiseBlue(coord + pk_FrameIndex.x, pk_FrameIndex.x); 
    n.xy -= 0.5f;
    n.z = NoiseUniformToTriangle(n.z);
    return n;
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

float4 SampleVolumeFog(float2 uv, float linearDepth)
{
	float w = GetVolumeWCoord(linearDepth);

	float2 dither = GlobalNoiseBlue(uint2(uv * pk_ScreenSize.xy), pk_FrameIndex.x).xy;
    dither -= 0.5f;
    dither *= 2.0f.xx / VOLUME_SIZE_XY;

    float3 uvw = float3(uv, w) + float3(dither, 0.0f);

    return SAMPLE_TRICUBIC(pk_Volume_ScatterRead, uvw);
}
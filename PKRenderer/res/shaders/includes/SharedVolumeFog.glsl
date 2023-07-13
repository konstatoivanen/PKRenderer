#pragma once
#include GBuffers.glsl
#include Noise.glsl
#include BlueNoise.glsl
#include SceneEnv.glsl
#include TricubicSampler.glsl

#define VOLUMEFOG_XY_ALIGNMENT 8u
#define VOLUMEFOG_SIZE_Z 128
#define VOLUMEFOG_SIZE_Z_INV 0.0078125f // 1.0f / 128.0f
#define VOLUMEFOG_SIZE_XY uint2(pk_ScreenSize.xy / VOLUMEFOG_XY_ALIGNMENT)
#define VOLUMEFOG_SIZE uint3(pk_ScreenSize.xy / VOLUMEFOG_XY_ALIGNMENT, 128)
#define VOLUMEFOG_MIN_DENSITY 1e-5f
#define VOLUMEFOG_ACCUMULATION 0.5f

PK_DECLARE_CBUFFER(pk_Fog_Parameters, PK_SET_SHADER)
{
    float4 pk_Fog_Albedo;
    float4 pk_Fog_Absorption;
    float4 pk_Fog_WindDirSpeed;
    float pk_Fog_Anisotropy;
    float pk_Fog_DensityConstant;
    float pk_Fog_DensityHeightExponent;
    float pk_Fog_DensityHeightOffset;
    float pk_Fog_DensityHeightAmount;
    float pk_Fog_DensityNoiseAmount;
    float pk_Fog_DensityNoiseScale;
    float pk_Fog_DensityAmount;
};

PK_DECLARE_SET_SHADER uniform sampler3D pk_Fog_ScatterRead;
PK_DECLARE_SET_SHADER uniform sampler3D pk_Fog_InjectRead;
PK_DECLARE_SET_SHADER uniform sampler3D pk_Fog_DensityRead;
PK_DECLARE_SET_SHADER uniform sampler3D pk_Fog_TransmittanceRead;
layout(r32ui, set = PK_SET_SHADER) uniform uimage3D pk_Fog_Inject;
layout(r16f, set = PK_SET_SHADER) uniform image3D pk_Fog_Density;
layout(r32ui, set = PK_SET_SHADER) uniform uimage3D pk_Fog_Scatter;
layout(r11f_g11f_b10f, set = PK_SET_SHADER) uniform image3D pk_Fog_Transmittance;

DEFINE_TRICUBIC_SAMPLER(pk_Fog_ScatterRead, VOLUMEFOG_SIZE)

DEFINE_TRICUBIC_SAMPLER(pk_Fog_DensityRead, VOLUMEFOG_SIZE)

DEFINE_TRICUBIC_SAMPLER(pk_Fog_InjectRead, VOLUMEFOG_SIZE)

DEFINE_TRICUBIC_SAMPLER(pk_Fog_TransmittanceRead, VOLUMEFOG_SIZE)

float VolumeFog_CalculateDensity(float3 pos)
{
    float density = pk_Fog_DensityConstant;
    density += min(exp(pk_Fog_DensityHeightExponent * (-pos.y + pk_Fog_DensityHeightOffset)) * pk_Fog_DensityHeightAmount, 1e+3f);
    density *= NoiseScroll(pos, pk_Time.y * pk_Fog_WindDirSpeed.w, pk_Fog_DensityNoiseScale, pk_Fog_WindDirSpeed.xyz, pk_Fog_DensityNoiseAmount, -0.3, 8.0);
    return max(density * pk_Fog_DensityAmount, 0.0f);
}

float VolumeFog_CalculateDensitySky(float viewy, float far)
{
    float density = pk_Fog_DensityConstant;
    density += min(exp(pk_Fog_DensityHeightExponent * -viewy * far) * pk_Fog_DensityHeightAmount, 1e+3f);
    density = max(density * pk_Fog_DensityAmount, VOLUMEFOG_MIN_DENSITY);
    return density;
}

float VolumeFog_MarchTransmittance(const float3 origin, const float3 direction, const float dither, const float tMax)
{
    const uint StepCount = 4u;
    const float MaxStepSize = 0.25f;

    float mstep = clamp(tMax / StepCount, 0.0f, MaxStepSize);
    float prev_density = VolumeFog_CalculateDensity(origin);
    float transmittance = 1.0f;

    for (uint j = 0; j < 4; ++j)
    {
        const float3 pos = origin + direction * j * mstep + mstep * dither;
        const float density = VolumeFog_CalculateDensity(pos);
        transmittance *= exp(-lerp(prev_density, density, 0.5f) * mstep);
        prev_density = density;
    }

    return transmittance;
}

float VolumeFog_GetAccumulation(float3 uvw)
{
    return pk_FrameIndex.y == 0u || uvw.z < 0.0f || Any_NotEqual(saturate(uvw.xy), uvw.xy) ? 1.0f : VOLUMEFOG_ACCUMULATION;
}

float3 VolumeFog_WorldToPrevUVW(float3 worldpos)
{
    float3 uvw = ClipToUVW(mul(pk_MATRIX_L_VP_N, float4(worldpos, 1.0f)));
    uvw.z = ClipDepthExp(ViewDepth(uvw.z));
    return uvw;
}

void VolumeFog_Apply(float2 uv, float viewDepth, inout float3 color)
{
	float3 uvw = float3(uv, ClipDepthExp(viewDepth));
	float2 dither = GlobalNoiseBlue(uint2(uv * pk_ScreenSize.xy), pk_FrameIndex.x).xy;
    uvw.xy += (dither - 0.5f) * 2.0f.xx / VOLUMEFOG_SIZE_XY;

    float3 scatter = SAMPLE_TRICUBIC(pk_Fog_ScatterRead, uvw).rgb;
    float3 transmittance = SAMPLE_TRICUBIC(pk_Fog_TransmittanceRead, uvw).rgb;

    color = scatter + color * transmittance;
}

void VolumeFog_ApplySky(float3 viewdir, inout float3 color)
{
    const float far = pk_ProjectionParams.y;
    const float density = VolumeFog_CalculateDensitySky(viewdir.y, far);
    const float occlusion = viewdir.y * 0.5f + 0.5f;
    const float3 irradiance = pk_Fog_Albedo.rgb * occlusion * SampleEnvironmentSHVolumetric(viewdir, pk_Fog_Anisotropy);
    const float3 transmittance = exp(-density * pk_Fog_Absorption.rgb * far);
    color = irradiance * (1.0f - transmittance) + color * transmittance;
}
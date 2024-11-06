#pragma once
#include "GBuffers.glsl"
#include "Noise.glsl"
#include "NoiseBlue.glsl"
#include "SceneEnv.glsl"
#include "TricubicSampler.glsl"
#include "Lighting.glsl"

#define VOLUMEFOG_XY_ALIGNMENT 8u
#define VOLUMEFOG_SIZE_Z 128
#define VOLUMEFOG_SIZE_Z_INV 0.0078125f // 1.0f / 128.0f
#define VOLUMEFOG_SIZE_XY uint2(pk_ScreenSize.xy / VOLUMEFOG_XY_ALIGNMENT)
#define VOLUMEFOG_SIZE uint3(pk_ScreenSize.xy / VOLUMEFOG_XY_ALIGNMENT, 128)
#define VOLUMEFOG_MIN_DENSITY 1e-5f
#define VOLUMEFOG_ACCUMULATION 0.5f
#define VOLUMEFOG_MARCH_DISTANCE_EXP 2.0f

PK_DECLARE_SET_SHADER uniform sampler3D pk_Fog_ScatterRead;
PK_DECLARE_SET_SHADER uniform sampler3D pk_Fog_InjectRead;
PK_DECLARE_SET_SHADER uniform sampler3D pk_Fog_DensityRead;
PK_DECLARE_SET_SHADER uniform sampler3D pk_Fog_ExtinctionRead;
layout(r32ui, set = PK_SET_SHADER) uniform uimage3D pk_Fog_Inject;
layout(r16f, set = PK_SET_SHADER) uniform image3D pk_Fog_Density;
layout(rgba16f, set = PK_SET_SHADER) uniform image3D pk_Fog_Scatter;

DEFINE_TRICUBIC_SAMPLER(pk_Fog_ScatterRead, VOLUMEFOG_SIZE)

DEFINE_TRICUBIC_SAMPLER(pk_Fog_DensityRead, VOLUMEFOG_SIZE)

DEFINE_TRICUBIC_SAMPLER(pk_Fog_InjectRead, VOLUMEFOG_SIZE)

// Direct exp transform packs too much resolution to near clip. linearize a bit with sqrt
float VFog_ZToView(float z) { return ViewDepthExp(sqrt(z)); }
float VFog_ViewToZ(float viewdepth) { return pow2(ClipDepthExp(viewdepth)); }

float VFog_CalculateDensity(float3 pos)
{
    float density = pk_Fog_Density_Constant;
    density += min(exp(pk_Fog_Density_HeightExponent * (-pos.y + pk_Fog_Density_HeightOffset)) * pk_Fog_Density_HeightAmount, 1e+3f);
    density *= NoiseScroll(pos, pk_Time.y * pk_Fog_WindDirSpeed.w, pk_Fog_Density_NoiseScale, pk_Fog_WindDirSpeed.xyz, pk_Fog_Density_NoiseAmount, -0.3, 8.0);
    return max(density * pk_Fog_Density_Amount, 0.0f);
}

float3 VFog_MarchTransmittance(const float3 origin, const float3 direction, const float dither, const float tMax, const float farFade)
{
    const float invT = tMax / 4.0f;
    float prev_density = VFog_CalculateDensity(origin);
    float extinction = 0.0f;

    for (uint j = 0u; j < 4u; ++j)
    {
        const float3 pos = origin + direction * invT * (j + dither);
        const float density = VFog_CalculateDensity(pos);
        extinction += lerp(prev_density, density, 0.5f) * invT;
        prev_density = density;
    }

    return exp(-extinction * pk_Fog_Absorption.rgb * farFade);
}

// transmittance estimator for indirect lighting.
float3 VFog_EstimateTransmittance(const float3 uvw, float farFade)
{
    const float depthMin = VFog_ZToView(uvw.z);
    const float depthMax = VFog_ZToView(min(1.0f, uvw.z + VOLUMEFOG_SIZE_Z_INV));
    const float density = texture(pk_Fog_DensityRead, uvw).x;
    const float extinction = density * (depthMax - depthMin) * farFade;
    return exp(-extinction * pk_Fog_Absorption.rgb);
}

float VFog_GetAccumulation(float3 uvw)
{
    return pk_FrameIndex.y == 0u || uvw.z < 0.0f || Any_NotEqual(saturate(uvw.xy), uvw.xy) ? 1.0f : VOLUMEFOG_ACCUMULATION;
}

float3 VFog_WorldToPrevUVW(float3 worldpos)
{
    float3 uvw = ClipToUVW(pk_WorldToClipPrev_NoJitter * float4(worldpos, 1.0f));
    uvw.z = VFog_ViewToZ(ViewDepth(uvw.z));
    return uvw;
}

float4 VFog_Apply(float2 uv, float viewDepth, float3 color)
{
    float3 uvw = float3(uv, VFog_ViewToZ(viewDepth));
    float2 dither = GlobalNoiseBlue(uint2(uv * pk_ScreenSize.xy), pk_FrameIndex.x).xy;
    uvw.xy += (dither - 0.5f) * 2.0f.xx / VOLUMEFOG_SIZE_XY;

    const float4 scatter = SAMPLE_TRICUBIC(pk_Fog_ScatterRead, uvw);
    // Reconstruct extinction & apply absorption (this leads to some bias due to floating point precision).
    const float3 transmittance = exp(log(scatter.a) * pk_Fog_Absorption.rgb);

    return float4(scatter.rgb + color * transmittance, dot(transmittance, 0.333f.xxx));
}

void VFog_GetSky(float3 viewdir, inout float3 irradiance, inout float3 transmittance)
{
    float density = pk_Fog_Density_Sky_Constant;
    density += min(exp(pk_Fog_Density_Sky_HeightExponent * -(viewdir.y + pk_Fog_Density_Sky_HeightOffset)) * pk_Fog_Density_Sky_HeightAmount, 1e+3f);
    density = density * pk_Fog_Density_Amount;

    const float occlusion = viewdir.y * 0.5f + 0.5f;
    irradiance = pk_Fog_Albedo.rgb * occlusion * SampleEnvironmentSHVolumetric(viewdir, pk_Fog_Phase1);

    // @TODO refactor to use somekind of global light cluster for this.
    // For now get the first light as it is likely a directional light
    const LightPacked light = Lights_LoadPacked(0u);

    if ((light.LIGHT_TYPE) == LIGHT_TYPE_DIRECTIONAL)
    {
        irradiance += BxDF_Volumetric(viewdir, pk_Fog_Phase0, pk_Fog_Phase1, pk_Fog_PhaseW, -light.LIGHT_POS, light.LIGHT_COLOR, 1.0f);
    }

    const float virtualDistance = 1000.0f;
    transmittance = exp(-density * pk_Fog_Absorption.rgb * virtualDistance);
}

float3 VFog_ApplySky(float3 viewdir, float3 color)
{
    float3 irrad, trans;
    VFog_GetSky(viewdir, irrad, trans);
    return lerp(irrad, color, trans);
}
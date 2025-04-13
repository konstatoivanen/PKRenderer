#pragma once
#include "GBuffers.glsl"
#include "Noise.glsl"
#include "NoiseBlue.glsl"
#include "SceneEnv.glsl"
#include "TricubicSampler.glsl"
#include "BRDF.glsl"
#include "LightSampling.glsl"

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
PK_DECLARE_SET_SHADER uniform uimage3D pk_Fog_Inject;
PK_DECLARE_SET_SHADER uniform image3D pk_Fog_Density;
PK_DECLARE_SET_SHADER uniform image3D pk_Fog_Scatter;

DEFINE_TRICUBIC_SAMPLER(pk_Fog_ScatterRead, VOLUMEFOG_SIZE)

DEFINE_TRICUBIC_SAMPLER(pk_Fog_DensityRead, VOLUMEFOG_SIZE)

DEFINE_TRICUBIC_SAMPLER(pk_Fog_InjectRead, VOLUMEFOG_SIZE)

// Direct exp transform packs too much resolution to near clip. linearize a bit with sqrt
// @TODO optimize by moving all this calc to separate viewdpeth exp params.
float Fog_ZToView(float z) { return ViewDepthExp(sqrt(z)) * pk_Fog_ZFarMultiplier; }
float Fog_ViewToZ(float view_depth) { return pow2(ClipDepthExp(view_depth / pk_Fog_ZFarMultiplier)); }

float Fog_VolumetricToStaticFade(float uvz) { return pow2(saturate(uvz * 4.0f - 3.0f)); }

float Fog_CalculateDensity(float3 pos)
{
    float d = pk_Fog_Density_Constant;
    d += min(exp(pk_Fog_Density_HeightExponent * (-pos.y + pk_Fog_Density_HeightOffset)) * pk_Fog_Density_HeightAmount, 1e+3f);
    d *= NoiseScroll(pos, pk_Time.y * pk_Fog_WindDirSpeed.w, pk_Fog_Density_NoiseScale, pk_Fog_WindDirSpeed.xyz, pk_Fog_Density_NoiseAmount, -0.3, 8.0);
    return max(d * pk_Fog_Density_Amount, 0.0f);
}

// Source: https://advances.realtimerendering.com/s2017/DecimaSiggraph2017.pdf
float Fog_CalculateStaticDensity(const float origin_y, const float surf_y, const float exponent, const float offset, const float heightAmount, const float constant)
{
    float d;
    d = max(1e-2f, (origin_y - surf_y) * exponent);
    d = (1.0f - exp(-d)) / d * exp((-surf_y + offset) * exponent);
    d *= heightAmount;
    d += constant;
    return d;
}

float3 Fog_MarchTransmittance(const float3 origin, const float3 direction, const float dither, const float t_max, const float fade_static)
{
    const float inv_t = t_max / 4.0f;
    float prev_density = Fog_CalculateDensity(origin);
    float extinction = 0.0f;

    for (uint j = 0u; j < 4u; ++j)
    {
        const float3 pos = origin + direction * inv_t * (j + dither);
        const float density = Fog_CalculateDensity(pos);
        extinction += lerp(prev_density, density, 0.5f) * inv_t;
        prev_density = density;
    }

    return exp(-extinction * pk_Fog_Absorption.rgb * fade_static);
}

// transmittance estimator for indirect lighting.
float3 Fog_EstimateTransmittance(const float3 uvw, float fade_static)
{
    const float depth_min = Fog_ZToView(uvw.z);
    const float depth_max = Fog_ZToView(min(1.0f, uvw.z + VOLUMEFOG_SIZE_Z_INV));
    const float density = texture(pk_Fog_DensityRead, uvw).x;
    const float extinction = density * (depth_max - depth_min) * fade_static;
    return exp(-extinction * pk_Fog_Absorption.rgb);
}

float Fog_GetAccumulation(float3 uvw)
{
    return VOLUMEFOG_ACCUMULATION + float(Any_NotEqual(clamp(uvw, 0.0f.xxx, float3(1.0f.xx, 2.0f)), uvw)) * (1.0f - VOLUMEFOG_ACCUMULATION);
}

float3 Fog_WorldToPrevUvw(float3 world_pos)
{
    float3 uvw = ClipToUvw(pk_WorldToClipPrev_NoJitter * float4(world_pos, 1.0f));
    uvw.z = Fog_ViewToZ(ViewDepth(uvw.z));
    return uvw;
}

void Fog_SampleStatic(float3 origin, float3 view_dir, float view_depth, inout float3 scatter, inout float3 transmittance)
{
    float o_y = origin.y;
    float v_y = origin.y + view_dir.y * view_depth;

    const float s0_density = Fog_CalculateStaticDensity(
        o_y, v_y, 
        pk_Fog_Density_Sky_HeightExponent, 
        pk_Fog_Density_Sky_HeightOffset, 
        pk_Fog_Density_Sky_HeightAmount, 
        pk_Fog_Density_Sky_Constant);

    const float s1_density = Fog_CalculateStaticDensity(
        o_y, v_y, 
        pk_Fog_Density_HeightExponent, 
        pk_Fog_Density_HeightOffset, 
        pk_Fog_Density_HeightAmount, 
        pk_Fog_Density_Constant);

    const float density = (s0_density + s1_density) * pk_Fog_Density_Amount;
    transmittance = exp(-density * pk_Fog_Absorption.rgb * view_depth);

    const float occlusion = view_dir.y * 0.5f + 0.5f;
    scatter = pk_Fog_Albedo.rgb * occlusion * SampleEnvironmentSHVolumetric(view_dir, pk_Fog_Phase1);

    // @TODO refactor to use somekind of global light cluster for this.
    // For now get the first light as it is likely a directional light
    const LightPacked light = Lights_LoadPacked(0u);

    if ((light.LIGHT_TYPE) == LIGHT_TYPE_DIRECTIONAL)
    {
        scatter += BxDF_Volumetric(view_dir, pk_Fog_Phase0, pk_Fog_Phase1, pk_Fog_PhaseW, -light.LIGHT_POS, light.LIGHT_COLOR, 1.0f);
    }
}

float4 Fog_SampleFroxel(float2 uv, float view_depth, float3 color)
{
    float3 uvw = float3(uv, Fog_ViewToZ(view_depth));
    float2 dither = GlobalNoiseBlue(uint2(uv * pk_ScreenSize.xy), pk_FrameIndex.x).xy;
    uvw.xy += (dither - 0.5f) * 2.0f.xx / VOLUMEFOG_SIZE_XY;

    float4 scatter = SAMPLE_TRICUBIC(pk_Fog_ScatterRead, uvw);
    // Reconstruct extinction & apply absorption (this leads to some bias due to floating point precision).
    float3 transmittance = exp(log(scatter.a) * pk_Fog_Absorption.rgb);

    // Sample static fog as a fallback for far away surfaces.
    // Skip sky pixels as they have baked fogging from sky capture.
    if (Test_DepthIsScene(view_depth))
    {
        float3 s_scatter; 
        float3 s_transmittance;
        float3 world_view_dir = ViewToWorldVec(UvToViewDir(uv));

        Fog_SampleStatic(pk_ViewWorldOrigin.xyz, world_view_dir, view_depth, s_scatter, s_transmittance);

        const float fade = Fog_VolumetricToStaticFade(uvw.z);
        scatter.rgb += transmittance * s_scatter * fade;
        transmittance *= lerp(1.0f.xxx, s_transmittance, fade);
    }

    return float4(scatter.rgb + color * transmittance, dot(transmittance, 0.333f.xxx));
}


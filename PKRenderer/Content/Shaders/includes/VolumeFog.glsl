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
#define VOLUMEFOG_FADE_START_SHADOW_DIRECT 0.95f
#define VOLUMEFOG_FADE_START_SHADOW_SELF 0.75f

PK_DECLARE_SET_SHADER uniform sampler3D pk_Fog_Scatter_Read;
PK_DECLARE_SET_SHADER uniform sampler3D pk_Fog_Inject_Read;
PK_DECLARE_SET_SHADER uniform sampler3D pk_Fog_Density_Read;
PK_DECLARE_SET_SHADER uniform uimage3D pk_Fog_Inject_Write;
PK_DECLARE_SET_SHADER uniform image3D pk_Fog_Density_Write;
PK_DECLARE_SET_SHADER uniform image3D pk_Fog_Scatter_Write;

DEFINE_TRICUBIC_SAMPLER(pk_Fog_Scatter_Read, VOLUMEFOG_SIZE)

DEFINE_TRICUBIC_SAMPLER(pk_Fog_Density_Read, VOLUMEFOG_SIZE)

DEFINE_TRICUBIC_SAMPLER(pk_Fog_Inject_Read, VOLUMEFOG_SIZE)

float Fog_ZToView(float z) { return ViewDepthExp(z, pk_Fog_ZParams.xyz); }
float Fog_ViewToZ(float z) { return ClipDepthExp(z, pk_Fog_ZParams.xyz); }

float Fog_Fade_FroxelShadows_Direct(float view_dist) 
{ 
    return saturate((pk_Fog_ZParams.w - view_dist) * pk_Fog_FadeParams.x); 
}

float Fog_Fade_FroxelShadows_Volumetric(float view_dist) 
{ 
    return saturate((pk_Fog_ZParams.w - view_dist) * pk_Fog_FadeParams.y); 
}

float Fog_Fade_Static(float view_dist) 
{ 
    return 1.0f - saturate((pk_Fog_ZParams.w - view_dist) * pk_Fog_FadeParams.z); 
}

float Fog_StaticOcclusion(float3 view_dir) { return exp(min(0.0f, view_dir.y) * pk_Fog_FadeParams.w); }

float Fog_CalculateDensity(float3 pos)
{
    float d = pk_Fog_Density_ExpParams0.x;
    d += min(exp(pk_Fog_Density_ExpParams0.y * (-pos.y + pk_Fog_Density_ExpParams0.z)) * pk_Fog_Density_ExpParams0.w, 1e+3f);
    d *= NoiseScroll(pos, pk_Time.y * pk_Fog_WindDirSpeed.w, pk_Fog_Density_NoiseScale, pk_Fog_WindDirSpeed.xyz, pk_Fog_Density_NoiseAmount, -0.3, 8.0);
    return max(d * pk_Fog_Density_Amount, 0.0f);
}

// Source: https://advances.realtimerendering.com/s2017/DecimaSiggraph2017.pdf
float Fog_CalculateStaticDensity(const float origin_y, const float surf_y, const float4 params)
{
    float d;
    d = max(1e-2f, (origin_y - surf_y) * params.y);
    d = (1.0f - exp(-d)) / d * exp((-surf_y + params.z) * params.y);
    d *= params.w;
    d += params.x;
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
    const float density = texture(pk_Fog_Density_Read, uvw).x;
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

    const float s0_density = Fog_CalculateStaticDensity(o_y, v_y, pk_Fog_Density_ExpParams0);
    const float s1_density = Fog_CalculateStaticDensity(o_y, v_y, pk_Fog_Density_ExpParams1);
    const float density = (s0_density + s1_density) * pk_Fog_Density_Amount;
    transmittance = exp(-density * pk_Fog_Absorption.rgb * view_depth);

    const float occlusion = Fog_StaticOcclusion(view_dir);
    scatter = occlusion * SceneEnv_Sample_ISL(EncodeOctaUv(view_dir), 0.0f);

    // @TODO refactor to use somekind of global light cluster for this.
    // For now get the first light as it is likely a directional light
    const SceneLight light = Lights_LoadLight(0u);

    if ((light.light_type) == LIGHT_TYPE_DIRECTIONAL)
    {
        scatter += BxDF_Volumetric(view_dir, pk_Fog_Phase0, pk_Fog_Phase1, pk_Fog_PhaseW, -light.position, light.color, 1.0f);
    }

    scatter *= pk_Fog_Albedo.rgb;
}

float4 Fog_SampleFroxel(float2 uv, float view_depth, float3 color)
{
    float3 uvw = float3(uv, Fog_ViewToZ(view_depth));
    float2 dither = GlobalNoiseBlue(uint2(uv * pk_ScreenSize.xy), pk_FrameIndex.x).xy;
    uvw.xy += (dither - 0.5f) * 2.0f.xx / VOLUMEFOG_SIZE_XY;

    float4 scatter = SAMPLE_TRICUBIC(pk_Fog_Scatter_Read, uvw);
    // Reconstruct extinction & apply absorption (this leads to some bias due to floating point precision).
    float3 transmittance = exp(log(scatter.a) * pk_Fog_Absorption.rgb);

    // Sample static fog as a fallback for far away surfaces.
    // Skip sky pixels as they have baked fogging from sky capture.
    if (Test_DepthIsScene(view_depth))
    {
        float3 s_scatter; 
        float3 s_transmittance;
        float3 view_vector = UvToViewPos(uv, view_depth);
        float view_dist = length(view_vector);
        float3 world_view_dir = ViewToWorldVec(view_vector / view_dist);

        Fog_SampleStatic(pk_ViewWorldOrigin.xyz, world_view_dir, view_dist, s_scatter, s_transmittance);

        const float fade = Fog_Fade_Static(view_dist);
        scatter.rgb += transmittance * s_scatter * fade;
        transmittance *= lerp(1.0f.xxx, s_transmittance, fade);
    }

    return float4(scatter.rgb + color * transmittance, dot(transmittance, 0.333f.xxx));
}


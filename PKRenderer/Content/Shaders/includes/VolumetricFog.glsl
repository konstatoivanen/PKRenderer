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

uniform sampler3D pk_Fog_Scatter_Read;
uniform sampler3D pk_Fog_Inject_Read;
uniform sampler3D pk_Fog_Density_Read;
uniform uimage3D pk_Fog_Inject_Write;
uniform image3D pk_Fog_Density_Write;
uniform image3D pk_Fog_Scatter_Write;

DEFINE_TRICUBIC_SAMPLER(pk_Fog_Scatter_Read, VOLUMEFOG_SIZE)

DEFINE_TRICUBIC_SAMPLER(pk_Fog_Density_Read, VOLUMEFOG_SIZE)

DEFINE_TRICUBIC_SAMPLER(pk_Fog_Inject_Read, VOLUMEFOG_SIZE)

float Fog_ZToView(float z) { return ViewDepthExp(z, pk_Fog_ZParams.xyz); }
float Fog_ViewToZ(float z) { return ClipDepthExp(z, pk_Fog_ZParams.xyz); }
float Fog_Fade_Static(float view_dist) { return 1.0f - saturate((pk_Fog_ZParams.w - view_dist) * pk_Fog_FadeParams.z); }
float Fog_StaticOcclusion(float3 view_dir) { return exp(min(0.0f, view_dir.y) * pk_Fog_FadeParams.w); }

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



#pragma pk_multi_compile PASS_CLEAR PASS_DENSITY PASS_INJECT PASS_INTEGRATE PASS_COMPOSITE
#pragma pk_program Compute ClearCs PASS_CLEAR
#pragma pk_program Compute DensityCs PASS_DENSITY
#pragma pk_program Compute InjectCs PASS_INJECT
#pragma pk_program Compute IntegrateCs PASS_INTEGRATE
#pragma pk_program Compute CompositeCs PASS_COMPOSITE

#define EARLY_Z_TEST 1
#define SHADOW_TEST ShadowTest_PCF2x2
#define SHADOW_SAMPLE_VOLUMETRICS 1
#define VOLUMEFOG_ACCUMULATION 0.5f
#define VOLUMEFOG_MARCH_DISTANCE_EXP 2.0f

// Hacky way to pass correct prog coord to shadow pass without parameters.
// Needed as we swizzle the prog coord for wave ops.
static uint2 g_shadow_prog_coord;
#define SHADOW_PROG_COORD g_shadow_prog_coord

#include "includes/VolumetricFog.glsl"
#include "includes/SceneGIVX.glsl"
#include "includes/CTASwizzling.glsl"
#include "includes/Encoding.glsl"

float Fog_Fade_FroxelShadows_Direct(float view_dist)
{
    return saturate((pk_View.Fog_ZParams.w - view_dist) * pk_View.Fog_FadeParams.x);
}

float Fog_Fade_FroxelShadows_Volumetric(float view_dist)
{
    return saturate((pk_View.Fog_ZParams.w - view_dist) * pk_View.Fog_FadeParams.y);
}

float Fog_GetAccumulation(float3 uvw)
{
    return VOLUMEFOG_ACCUMULATION + float(any(clamp(uvw, 0.0f.xxx, float3(1.0f.xx, 2.0f)) != uvw)) * (1.0f - VOLUMEFOG_ACCUMULATION);
}

float3 Fog_WorldToPrevUvw(float3 world_pos)
{
    float3 uvw = ClipToUvw(mul(pk_View.WorldToClipPrev_NoJitter, float4(world_pos, 1.0f)));
    uvw.z = Fog_ViewToZ(ViewDepth(uvw.z));
    return uvw;
}

float Fog_CalculateDensity(float3 pos)
{
    float d = pk_View.Fog_Density_ExpParams0.x;
    d += min(exp(pk_View.Fog_Density_ExpParams0.y * (-pos.y + pk_View.Fog_Density_ExpParams0.z)) * pk_View.Fog_Density_ExpParams0.w, 1e+3f);
    d *= NoiseScroll(pos, pk_View.Time.y * pk_View.Fog_WindDirSpeed.w, pk_View.Fog_Density_NoiseScale, pk_View.Fog_WindDirSpeed.xyz, pk_View.Fog_Density_NoiseAmount, -0.3, 8.0);
    return max(d * pk_View.Fog_Density_Amount, 0.0f);
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

    return exp(-extinction * pk_View.Fog_Absorption.rgb * fade_static);
}

// transmittance estimator for indirect lighting.
float3 Fog_EstimateTransmittance(const float3 uvw, float fade_static)
{
    const float depth_min = Fog_ZToView(uvw.z);
    const float depth_max = Fog_ZToView(min(1.0f, uvw.z + VOLUMEFOG_SIZE_Z_INV));
    const float density = pk_Fog_Density_Read.SampleLevel(pk_SamplerBilinearClamped, uvw, 0);
    const float extinction = density * (depth_max - depth_min) * fade_static;
    return exp(-extinction * pk_View.Fog_Absorption.rgb);
}

[numthreads(4u, 4u, 4u)]
void ClearCs()
{
    const int3 coord = int3(gl_GlobalInvocationID);
    pk_Fog_Density_Write[coord] = 0.0f;
    pk_Fog_Inject_Write[coord] = 0u;
}

[numthreads(4u, 4u, 4u)]
void DensityCs()
{
    const int3 pos = int3(gl_GlobalInvocationID);
    const float3 dither = GlobalNoiseBlue(pos.xy, pk_View.FrameIndex.x);
    const float3 uvw_cur = (pos + dither) / VOLUMEFOG_SIZE;

    const float3 world_pos = UvToWorldPos(uvw_cur.xy, Fog_ZToView(uvw_cur.z));
    const float3 uvw_prev = Fog_WorldToPrevUvw(world_pos);

    const float value_cur = Fog_CalculateDensity(world_pos);
    const float value_pre = SAMPLE_TRICUBIC(pk_Fog_Density_Read, uvw_prev).x;
    const float value_out = lerp(value_pre, value_cur, Fog_GetAccumulation(uvw_prev));
    pk_Fog_Density_Write[pos] = -min(0.0f, -value_out);
}

[numthreads(4u, 4u, 4u)]
void InjectCs()
{
    // Subgroups are swizzled into a 2x4x4 pattern
    const uint thread = gl_SubgroupInvocationID;
    const uint3 wave_id = gl_WorkGroupID.xyz * uint3(2u, 1u, 1u) + uint3(gl_SubgroupID, 0, 0);
    const uint3 local_coord = uint3(thread / 16u, thread & 3u, (thread / 4u) & 3u);
    const uint3 coord = wave_id * uint3(2u, 4u, 4u) + local_coord;
    g_shadow_prog_coord = coord.xy;

#if EARLY_Z_TEST == 1
    {
        // load a 5x6 kernel of 16x16px depth values. This needs to cover the composite voxel radius of 2.5
        const uint thread_30 = thread % 30u;
        const int2 wave_z_coord = int2(wave_id.xy) * int2(1, 2) + int2(thread_30 % 5u, thread_30 / 5u) - 2;
        const float lane_zmax = SampleMaxZ(wave_z_coord, 4);
        const float wave_zmax = WaveActiveMax(lane_zmax);
        const float tile_zmax = Fog_ZToView((gl_WorkGroupID.z * 4.0f + 1.0f) * VOLUMEFOG_SIZE_Z_INV);

        [[branch]]
        if (WaveActiveAllTrue(wave_zmax < tile_zmax))
        {
            pk_Fog_Inject_Write[int3(coord)] = 0u;
            return;
        }
    }
#endif

    const float3 dither = GlobalNoiseBlue(coord.xy, pk_View.FrameIndex.x);
    const float3 uvw_cur = (coord + float3(0.5f.xx, dither.z)) / VOLUMEFOG_SIZE;

    // Light leak threshold
    const float tile_zmin = Fog_ZToView((coord.z - 1.5f) * VOLUMEFOG_SIZE_Z_INV);
    const float tile_zmax = SampleMaxZ(int2(coord.xy), 3);
    // Clamp cell to surface to prevent light leaks
    const float depth = min(Fog_ZToView(uvw_cur.z), select(tile_zmin < tile_zmax, tile_zmax, 1e+38f));

    const float3 world_pos = UvToWorldPos(uvw_cur.xy, depth);
    const float3 uvw_prev = Fog_WorldToPrevUvw(world_pos);
    const float  view_dist = length(world_pos - pk_View.ViewWorldOrigin.xyz);
    const float3 view_dir = (world_pos - pk_View.ViewWorldOrigin.xyz) / view_dist;

    const float3 gi_static = SceneEnv_Sample_ISL_Dual(EncodeOctaUv(view_dir), pow2(uvw_cur.z));
    const float4 gi_dynamic = GI_VX_SphereTrace_Diffuse(world_pos);

    // Fade values for properties not present in backgroung fog
    const float fade_shadow_direct = Fog_Fade_FroxelShadows_Direct(view_dist);
    const float fade_shadow_volumetric = Fog_Fade_FroxelShadows_Volumetric(view_dist);
    // Distant texels are less dense, trace a longer distance to retain some depth.
    const float depth_slice = Fog_ZToView((coord.z + 1.0f) * VOLUMEFOG_SIZE_Z_INV) - Fog_ZToView(coord.z * VOLUMEFOG_SIZE_Z_INV);
    const float march_distance_max = exp(uvw_cur.z * VOLUMEFOG_MARCH_DISTANCE_EXP);
    const float3 shadow_bias = view_dir * depth_slice * 0.5f;

    // Occlude ground as it should be lit mostly by dynamic gi.
    // Apply visibility mask from cone trace.
    const float gi_static_occlusion = Fog_StaticOcclusion(view_dir) * gi_dynamic.a;

    float3 value_cur = gi_static.rgb * gi_static_occlusion + gi_dynamic.rgb;

    // This is incorrect for the dynamic component. However, it introduces good depth to the colors so whatever.
    value_cur *= Fog_EstimateTransmittance(uvw_cur, fade_shadow_volumetric);

    LightTile tile = Lights_LoadTile_Coord(int2(gl_WorkGroupID.xy >> 1), depth);

    for (uint i = tile.start; i < tile.end; ++i)
    {
        // @TODO current 1spp shadow test for fog is prone to banding. implement better filter.
        SceneLightSample light = Lights_SampleTiled(i, world_pos, shadow_bias, tile.cascade);

        const float march_distance = min(light.linear_distance, march_distance_max);
        light.Rv *= Fog_MarchTransmittance(world_pos, light.Lv, dither.y, march_distance, fade_shadow_volumetric);
        light.shadow = lerp(1.0f, light.shadow, fade_shadow_direct);

        value_cur += BxDF_Volumetric
        (
            view_dir,
            pk_View.Fog_Phase0,
            pk_View.Fog_Phase1,
            pk_View.Fog_PhaseW,
            light.Lv,
            light.Rv,
            light.shadow
        );
    }

    // Note it is faster to solve tricubic here rather than in density reproject.
    const float accumulation = Fog_GetAccumulation(uvw_prev);
    const float3 value_pre = SAMPLE_TRICUBIC(pk_Fog_Inject_Read, uvw_prev).rgb;
    const float3 value_out = -min(0.0f.xxx, -lerp(value_pre, value_cur, accumulation));

    pk_Fog_Inject_Write[int3(coord)] = EncodeE5BGR9(value_out);
}

[numthreads(8u, 8u, 1u)]
void IntegrateCs()
{
    float4 accum_scatter = float4(0.0f.xxx, 1.0f);
    float3 accum_transmittance = 1.0f.xxx;

    int3 pos = int3(gl_GlobalInvocationID.xy, 0);

    for (; pos.z < VOLUMEFOG_SIZE_Z; ++pos.z)
    {
        const float depth_min = Fog_ZToView(pos.z * VOLUMEFOG_SIZE_Z_INV);
        const float depth_max = Fog_ZToView((pos.z + 1.0f) * VOLUMEFOG_SIZE_Z_INV);
        const float slice_width = depth_max - depth_min;

        const float  density = pk_Fog_Density_Read[pos];
        const float3 irradiance = pk_Fog_Inject_Read[pos].rgb * pk_View.Fog_Albedo.rgb;

        const float  extinction = density * slice_width;
        const float3 transmittance = exp(-extinction * pk_View.Fog_Absorption.rgb);
        const float3 integral = irradiance * (1.0f - transmittance) * accum_transmittance;

        accum_scatter.rgb += integral;
        // Store transmittance as it suffers less from fp16 reduction than extinction.
        accum_scatter.a *= exp(-extinction);
        accum_transmittance *= transmittance;

        pk_Fog_Scatter_Write[pos] = accum_scatter;
    }
}

uniform RWTexture2DRef<float4> pk_Image;

[numthreads(PK_W_ALIGNMENT_8, PK_W_ALIGNMENT_8, 1u)]
void CompositeCs()
{
    const int2 coord = int2(GetXTiledThreadID(PK_W_ALIGNMENT_8, PK_W_ALIGNMENT_8, 8u));
    const int2 size = pk_Image.GetSize();
    const float2 uv = float2(coord + 0.5f.xx) / float2(size);

    const float3 color = pk_Image[coord].rgb;
    const float4 color_transmittance = Fog_SampleFroxel(uv, SampleViewDepth(uv), color);

    pk_Image[coord] = color_transmittance;
}


#pragma pk_program SHADER_STAGE_COMPUTE main

#define EARLY_Z_TEST 1
#define SHADOW_TEST ShadowTest_PCF2x2
#define SHADOW_SAMPLE_VOLUMETRICS 1

#include "includes/VolumeFog.glsl"
#include "includes/SceneGIVX.glsl"

layout(local_size_x = PK_W_ALIGNMENT_4, local_size_y = PK_W_ALIGNMENT_4, local_size_z = PK_W_ALIGNMENT_4) in;
void main()
{
    const uint3 id = gl_GlobalInvocationID;
    const float3 dither = GlobalNoiseBlue(id.xy, pk_FrameIndex.x);

    const float3 uvw_cur = (id + float3(0.5f.xx, dither.z)) / VOLUMEFOG_SIZE;

    // Light leak threshold
    const float zmin = Fog_ZToView((id.z - 1.5f) * VOLUMEFOG_SIZE_Z_INV);
    const float zmax = SampleMaxZ(int2(id.xy), 3);
    // Clamp cell to surface to prevent light leaks
    const float depth = min(Fog_ZToView(uvw_cur.z), lerp(1e+38f, zmax, zmin < zmax));

#if EARLY_Z_TEST == 1
    const float4 max_depths = float4
    (
        SampleMaxZ(float2(id.xy + float2(-0.5f, -0.5f)) / VOLUMEFOG_SIZE_XY, 4),
        SampleMaxZ(float2(id.xy + float2(-0.5f, +1.5f)) / VOLUMEFOG_SIZE_XY, 4),
        SampleMaxZ(float2(id.xy + float2(+1.5f, +1.5f)) / VOLUMEFOG_SIZE_XY, 4),
        SampleMaxZ(float2(id.xy + float2(+1.5f, -0.5f)) / VOLUMEFOG_SIZE_XY, 4)
    );

    float max_tile = cmax(max_depths);

    [[branch]]
    if (max_tile < depth)
    {
        imageStore(pk_Fog_Inject, int3(id), uint4(0));
        return;
    }
#endif

    const float3 world_pos = UvToWorldPos(uvw_cur.xy, depth);
    const float3 uvw_prev = Fog_WorldToPrevUvw(world_pos);
    const float  view_dist = length(world_pos - pk_ViewWorldOrigin.xyz);
    const float3 view_dir = (world_pos - pk_ViewWorldOrigin.xyz) / view_dist;

    const float3 gi_static = SceneEnv_Sample_ISL_Dual(EncodeOctaUv(view_dir), pow2(uvw_cur.z));
    const float4 gi_dynamic = GI_SphereTrace_Diffuse(world_pos);

    // Fade values for properties not present in backgroung fog
    const float fade_shadow_direct = Fog_Fade_FroxelShadows_Direct(view_dist);
    const float fade_shadow_volumetric = Fog_Fade_FroxelShadows_Volumetric(view_dist);
    // Distant texels are less dense, trace a longer distance to retain some depth.
    const float march_distance_max = exp(uvw_cur.z * VOLUMEFOG_MARCH_DISTANCE_EXP);
    const float shadow_bias_range = Fog_ZToView((id.z + 1.0f) * VOLUMEFOG_SIZE_Z_INV) - Fog_ZToView(id.z * VOLUMEFOG_SIZE_Z_INV);
    const float3 shadow_bias = view_dir * shadow_bias_range * 0.5f;

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
        light.color *= Fog_MarchTransmittance(world_pos, light.direction, dither.y, march_distance, fade_shadow_volumetric);
        light.shadow = lerp(1.0f, light.shadow, fade_shadow_direct);

        value_cur += BxDF_Volumetric
        (
            view_dir,
            pk_Fog_Phase0,
            pk_Fog_Phase1,
            pk_Fog_PhaseW,
            light.direction,
            light.color,
            light.shadow
        );
    }

    // Note it is faster to solve tricubic here rather than in density reproject.
    const float3 value_pre = SAMPLE_TRICUBIC(pk_Fog_InjectRead, uvw_prev).rgb;

    const float accumulation = Fog_GetAccumulation(uvw_prev);

    float3 value_out = lerp(value_pre, value_cur, accumulation);

    // Remove potential NaNs.
    value_out = -min(-0.0f.xxx, -value_out);

    imageStore(pk_Fog_Inject, int3(id), EncodeE5BGR9(value_out).xxxx);
}

#extension GL_KHR_shader_subgroup_arithmetic : enable
#extension GL_KHR_shader_subgroup_shuffle : require
#pragma pk_program SHADER_STAGE_COMPUTE main

#define EARLY_Z_TEST 1
#define SHADOW_TEST ShadowTest_PCF2x2
#define SHADOW_SAMPLE_VOLUMETRICS 1

// Hacky way to pass correct prog coord to shadow pass without parameters.
// Needed as we swizzle the prog coord for wave ops.
uint2 g_shadow_prog_coord;
#define SHADOW_PROG_COORD g_shadow_prog_coord

#include "includes/VolumeFog.glsl"
#include "includes/SceneGIVX.glsl"

layout(local_size_x = PK_W_ALIGNMENT_4, local_size_y = PK_W_ALIGNMENT_4, local_size_z = PK_W_ALIGNMENT_4) in;
void main()
{
    // Subgroups are swizzled into a 2x4x4 pattern
    const uint thread = gl_SubgroupInvocationID;
    const uint3 wave_id = gl_WorkGroupID.xyz * uint3(2u, 1u, 1u) + uint3(gl_SubgroupID, 0, 0);
    const uint3 local_coord = uint3(thread / 16u, thread & 3u, (thread / 4u) & 3u);
    const uint3 coord = wave_id * uint3(2u, 4u, 4u) + local_coord;
    g_shadow_prog_coord = coord.xy;

#if EARLY_Z_TEST == 1
    {
        // load a 5x6 kernel of 16x16px depth values. the final filtering has a maximum voxel offset of 2.5.
        const uint thread_30 = thread % 30u;
        const int2 wave_z_coord = int2(wave_id.xy) * int2(1, 2) + int2(thread_30 % 5u, thread_30 / 5u) - 2;
        const float lane_zmax = SampleMaxZ(wave_z_coord, 4);
        const float wave_zmax = subgroupMax(lane_zmax);
        const float tile_zmax = Fog_ZToView((gl_WorkGroupID.z * 4.0f + 1.0f) * VOLUMEFOG_SIZE_Z_INV);

        [[branch]]
        if (subgroupAll(wave_zmax < tile_zmax))
        {
            imageStore(pk_Fog_Inject, int3(coord), uint4(0));
            return;
        }
    }
#endif

    const float3 dither = GlobalNoiseBlue(coord.xy, pk_FrameIndex.x);
    const float3 uvw_cur = (coord + float3(0.5f.xx, dither.z)) / VOLUMEFOG_SIZE;

    // Light leak threshold
    const float tile_zmin = Fog_ZToView((coord.z - 1.5f) * VOLUMEFOG_SIZE_Z_INV);
    const float tile_zmax = SampleMaxZ(int2(coord.xy), 3);
    // Clamp cell to surface to prevent light leaks
    const float depth = min(Fog_ZToView(uvw_cur.z), lerp(1e+38f, tile_zmax, tile_zmin < tile_zmax));

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
            pk_Fog_Phase0,
            pk_Fog_Phase1,
            pk_Fog_PhaseW,
            light.Lv,
            light.Rv,
            light.shadow
        );
    }

    // Note it is faster to solve tricubic here rather than in density reproject.
    const float3 value_pre = SAMPLE_TRICUBIC(pk_Fog_InjectRead, uvw_prev).rgb;

    const float accumulation = Fog_GetAccumulation(uvw_prev);

    float3 value_out = lerp(value_pre, value_cur, accumulation);

    // Remove potential NaNs.
    value_out = -min(-0.0f.xxx, -value_out);

    imageStore(pk_Fog_Inject, int3(coord), EncodeE5BGR9(value_out).xxxx);
}

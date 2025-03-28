
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
    const float zmin = VFog_ZToView((id.z - 1.5f) * VOLUMEFOG_SIZE_Z_INV);
    const float zmax = SampleMaxZ(int2(id.xy), 3);
    // Clamp cell to surface to prevent light leaks
    const float depth = min(VFog_ZToView(uvw_cur.z), lerp(1e+38f, zmax, zmin < zmax));

#if EARLY_Z_TEST == 1
    float4 maxdepths = float4
        (
            SampleMaxZ(float2(id.xy + float2(-0.5f, -0.5f)) / VOLUMEFOG_SIZE_XY, 4),
            SampleMaxZ(float2(id.xy + float2(-0.5f, +1.5f)) / VOLUMEFOG_SIZE_XY, 4),
            SampleMaxZ(float2(id.xy + float2(+1.5f, +1.5f)) / VOLUMEFOG_SIZE_XY, 4),
            SampleMaxZ(float2(id.xy + float2(+1.5f, -0.5f)) / VOLUMEFOG_SIZE_XY, 4)
            );

    float maxTile = cmax(maxdepths);

    [[branch]]
    if (maxTile < depth)
    {
        imageStore(pk_Fog_Inject, int3(id), uint4(0));
        return;
    }
#endif

    const float3 worldpos = UVToWorldPos(uvw_cur.xy, depth);
    const float3 uvw_prev = VFog_WorldToPrevUVW(worldpos);
    const float3 viewdir = normalize(worldpos - pk_ViewWorldOrigin.xyz);

    const float3 gi_static = SampleEnvironmentSHVolumetric(viewdir, pk_Fog_Phase1);
    const float4 gi_dynamic = GI_SphereTrace_Diffuse(worldpos);

    // Fade value for properties not present in backgroung fog
    const float farFade = smoothstep(1.0f, 0.7f, uvw_cur.z);
    // Distant texels are less dense, trace a longer distance to retain some depth.
    const float maxMarchDistance = exp(uvw_cur.z * VOLUMEFOG_MARCH_DISTANCE_EXP);
    const float shadowBiasRange = VFog_ZToView((id.z + 1.0f) * VOLUMEFOG_SIZE_Z_INV) - VFog_ZToView(id.z * VOLUMEFOG_SIZE_Z_INV);
    const float3 shadowBias = viewdir * shadowBiasRange * 0.5f;

    // Occlude ground as it should be lit mostly by dynamic gi.
    // Apply visibility mask from cone trace.
    const float gi_static_occlusion = (viewdir.y * 0.5f + 0.5f) * gi_dynamic.a;

    float3 value_cur = gi_static.rgb * gi_static_occlusion + gi_dynamic.rgb;

    // This is incorrect for the dynamic component. However, it introduces good depth to the colors so whatever.
    value_cur *= VFog_EstimateTransmittance(uvw_cur, farFade);

    LightTile tile = Lights_GetTile_COORD(int2(gl_WorkGroupID.xy >> 1), depth);

    for (uint i = tile.start; i < tile.end; ++i)
    {
        // @TODO current 1spp shadow test for fog is prone to banding. implement better filter.
        LightSample light = Lights_SampleTiled(i, worldpos, shadowBias, tile.cascade);

        const float marchDistance = min(light.linearDistance, maxMarchDistance);
        light.color *= VFog_MarchTransmittance(worldpos, light.direction, dither.y, marchDistance, farFade);
        light.shadow = lerp(1.0f, light.shadow, farFade);

        value_cur += BxDF_Volumetric
        (
            viewdir,
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

    const float accumulation = VFog_GetAccumulation(uvw_prev);

    float3 value_out = lerp(value_pre, value_cur, accumulation);

    // Remove potential NaNs.
    value_out = -min(-0.0f.xxx, -value_out);

    imageStore(pk_Fog_Inject, int3(id), EncodeE5BGR9(value_out).xxxx);
}
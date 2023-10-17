#version 460
#pragma PROGRAM_COMPUTE

#define EARLY_Z_TEST 1
#define SHADOW_TEST ShadowTest_PCF2x2

#include includes/VolumeFog.glsl
#include includes/SceneGIVX.glsl

layout(local_size_x = PK_W_ALIGNMENT_4, local_size_y = PK_W_ALIGNMENT_4, local_size_z = PK_W_ALIGNMENT_4) in;
void main()
{
    const uint3 id = gl_GlobalInvocationID;
    float3 dither = GlobalNoiseBlue(id.xy, pk_FrameIndex.x);

    const float3 uvw_cur = (id + float3(0.5f.xx, dither.z)) / VOLUMEFOG_SIZE;

    // Light leak threshold
    const float zmin = ViewDepthExp((id.z - 1.5f) * VOLUMEFOG_SIZE_Z_INV);
    const float zmax = SampleMaxZ(int2(id.xy), 3);
    // Clamp cell to surface to prevent light leaks
    const float depth = min(ViewDepthExp(uvw_cur.z), lerp(1e+38f, zmax, zmin < zmax));

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
        return;
    }
#endif

    const float3 worldpos = UVToWorldPos(uvw_cur.xy, depth);
    const float3 uvw_prev = VolumeFog_WorldToPrevUVW(worldpos);
    const float3 viewdir = normalize(worldpos - pk_WorldSpaceCameraPos.xyz);

    const float3 gi_static = SampleEnvironmentSHVolumetric(viewdir, pk_Fog_Phase1);
    const float4 gi_dynamic = GI_SphereTrace_Diffuse(worldpos);

    // Occlude ground as it should be lit mostly by dynamic gi.
    // Apply visibility mask from cone trace.
    const float gi_static_occlusion = (viewdir.y * 0.5f + 0.5f) * gi_dynamic.a;

    float3 value_cur = gi_static.rgb * gi_static_occlusion + gi_dynamic.rgb;
    
    // This is incorrect for the dynamic component. However, it introduces good depth to the colors so whatever.
    value_cur *= VolumeFog_MarchTransmittanceStatic(uvw_cur, dither.x);

    // Distant texels are less dense, trace a longer distance to retain some depth.
    const float maxMarchDistance = exp(uvw_cur.z * VOLUMEFOG_MARCH_DISTANCE_EXP);

    // Shadow cascades & transmittance march will cause a visible border between sky fog & volume fog.
    // Fade out shadow contribution near far plane.
    const float shadowFade = smoothstep(0.9f, 1.0f, uvw_cur.z);

    LightTile tile = Lights_GetTile_COORD(int2(gl_WorkGroupID.xy >> 1), depth);
    for (uint i = tile.start; i < tile.end; ++i)
    {
        // @TODO current 1spp shadow test for fog is prone to banding. implement better filter.
        Light light = GetLight(i, worldpos, -viewdir, tile.cascade);
        const float marchDistance = min(light.linearDistance, maxMarchDistance);
        light.shadow *= VolumeFog_MarchTransmittance(worldpos, light.direction, dither.y, marchDistance);
        light.shadow = lerp(light.shadow, 1.0f, shadowFade);
        value_cur += EvaluateBxDF_Volumetric(viewdir, pk_Fog_Phase0, pk_Fog_Phase1, pk_Fog_PhaseW, light.direction, light.color, light.shadow);
    }

    const float accumulation = VolumeFog_GetAccumulation(uvw_prev);
    const float3 value_pre = ReplaceIfResized(SAMPLE_TRICUBIC(pk_Fog_InjectRead, uvw_prev).rgb, 0.0f.xxx);
    float3 value_out = lerp(value_pre, value_cur, accumulation);
    value_out = Any_IsNaN(value_out) ? 0.0f.xxx : value_out;
    imageStore(pk_Fog_Inject, int3(id), EncodeE5BGR9(value_out).xxxx);
}
#version 460
#pragma PROGRAM_COMPUTE
#include includes/Lighting.glsl
#include includes/SharedVolumeFog.glsl
#include includes/SharedSceneGI.glsl

layout(local_size_x = PK_W_ALIGNMENT_4, local_size_y = PK_W_ALIGNMENT_4, local_size_z = PK_W_ALIGNMENT_4) in;
void main()
{
    const uint3 id = gl_GlobalInvocationID;
    const float3 dither = GlobalNoiseBlue(id.xy, pk_FrameIndex.x);
    const float3 uvw_cur = (id + dither) / VOLUMEFOG_SIZE;

    // Light leak threshold
    const float zmin = ViewDepthExp((id.z - 1.5f) * VOLUMEFOG_SIZE_Z_INV);
    const float zmax = SampleMaxZ(int2(id.xy), 3);
    // Clamp cell to surface to prevent light leaks
    const float depth = min(ViewDepthExp(uvw_cur.z), lerp(1e+38f, zmax, zmin < zmax));
    

    const float3 worldpos = UVToWorldPos(uvw_cur.xy, depth);
    const float3 uvw_prev = VolumeFog_WorldToPrevUVW(worldpos);
    const float3 viewdir  = normalize(worldpos - pk_WorldSpaceCameraPos.xyz);

    const float3 gi_static  = SampleEnvironmentSHVolumetric(viewdir, pk_Fog_Anisotropy);
    const float4 gi_dynamic = GI_SphereTrace_Diffuse(worldpos);

    // Occlude ground as it should be lit mostly by dynamic gi.
    // Apply visibility mask from cone trace.
    const float gi_static_occlusion = (viewdir.y * 0.5f + 0.5f) * gi_dynamic.a;

    float3 value_cur = gi_static.rgb * gi_static_occlusion + gi_dynamic.rgb;

    // Distant texels are less dense, trace a longer distance to retain some depth.
    const float maxMarchDistance = exp(uvw_cur.z * VOLUMEFOG_MARCH_DISTANCE_EXP);
    
    LightTile tile = GetLightTile(uvw_cur.xy, depth);
    for (uint i = tile.start; i < tile.end; ++i)
    {
        Light light = GetLight(i, worldpos, tile.cascade);
        const float marchDistance = clamp(light.linearDistance, 0.0f, maxMarchDistance);
        light.shadow *= VolumeFog_MarchTransmittance(worldpos, light.direction, dither.z, marchDistance);
        value_cur += BSDF_VOLUMETRIC(viewdir, pk_Fog_Anisotropy, light.direction, light.color, light.shadow);
    }

    const float accumulation = VolumeFog_GetAccumulation(uvw_prev);
    const float3 value_pre = ReplaceIfResized(SAMPLE_TRICUBIC(pk_Fog_InjectRead, uvw_prev).rgb, 0.0f.xxx);
    float3 value_out = lerp(value_pre, value_cur, accumulation);
    value_out = Any_IsNaN(value_out) ? 0.0f.xxx : value_out;
    imageStore(pk_Fog_Inject, int3(id), uint4(EncodeE5BGR9(value_out)));
}
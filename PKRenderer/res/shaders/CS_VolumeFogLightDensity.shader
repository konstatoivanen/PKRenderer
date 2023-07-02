#version 460
#pragma PROGRAM_COMPUTE
#include includes/Lighting.glsl
#include includes/SceneEnv.glsl
#include includes/SharedVolumeFog.glsl
#include includes/SharedSceneGI.glsl

float Density(float3 pos)
{
    float fog = pk_Volume_ConstantFog;

    fog += clamp(exp(pk_Volume_HeightFogExponent * (-pos.y + pk_Volume_HeightFogOffset)) * pk_Volume_HeightFogAmount, 0.0, 1e+3f);

    float3 warp = pos;

    fog *= NoiseScroll(warp, pk_Time.y * pk_Volume_WindSpeed, pk_Volume_NoiseFogScale, pk_Volume_WindDir.xyz, pk_Volume_NoiseFogAmount, -0.3, 8.0);

    return max(fog * pk_Volume_Density, 0.0f);
}

layout(local_size_x = PK_W_ALIGNMENT_4, local_size_y = PK_W_ALIGNMENT_4, local_size_z = PK_W_ALIGNMENT_4) in;
void main()
{
    const uint3 id = gl_GlobalInvocationID;
    const float3 dither = GetVolumeCellDither(id.xy);
    const float2 uv = (id.xy + 0.5f.xx + dither.xy) / VOLUME_SIZE_XY;

    // Triangle dither range is -1.5 - 1.5 and due to trilinear interpolation we also need to have 2 texels worth of coverage.
    const float zmin = GetVolumeCellDepth(id.z - 3.0f);
    const float zmax = max(SampleMaxZ(uv, 3), SampleMaxZ(int2(id.xy), 3));
    float depth = GetVolumeCellDepth(id.z + dither.x);

    // Texel is inside dither & trilinear interpolation range. Let's clamp it so that we can avoid light leaking through thin surfaces.
    if (zmin < zmax)
    {
        depth = min(zmax, depth);
    }

    depth = max(pk_ProjectionParams.x, depth);

    const float3 worldpos = mul(pk_MATRIX_I_V, float4(ClipUVToViewPos(uv, depth), 1.0f)).xyz;
    const float3 uvw_prev = ReprojectWorldToCoord(worldpos);
    const float3 viewdir = normalize(worldpos - pk_WorldSpaceCameraPos.xyz);

    const float3 gi_static = SampleEnvironmentSHVolumetric(viewdir, pk_Volume_Anisotropy);
    const float4 gi_dynamic = GI_ConeTrace_Volumetric(worldpos);

    // Occlude ground as it should be lit mostly by dynamic gi.
    // Apply visibility mask from cone trace.
    const float gi_static_occlusion = (viewdir.y * 0.5f + 0.5f) * gi_dynamic.a;

    float3 radiance = gi_static.rgb * gi_static_occlusion + gi_dynamic.rgb;

    LightTile tile = GetLightTile(GetTileIndexUV(uv, depth));

    for (uint i = tile.start; i < tile.end; ++i)
    {
        radiance += GetVolumeLightColor(i, worldpos, viewdir, tile.cascade, pk_Volume_Anisotropy);
    }

    const float density = Density(worldpos);
    const float accumulation = GetVolumeAccumulation(uvw_prev);

    const float4 value_pre = ReplaceIfResized(SAMPLE_TRICUBIC(pk_Volume_InjectRead, uvw_prev), 0.0f.xxxx);
    const float4 value_cur = float4(pk_Volume_Intensity * density * radiance, density);
    const float4 value_out = lerp(value_pre, value_cur, accumulation);

    imageStore(pk_Volume_Inject, int3(id), Any_IsNaN(value_out) ? 0.0f.xxxx : value_out);
}
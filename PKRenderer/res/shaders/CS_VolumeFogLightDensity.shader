#version 460
#pragma PROGRAM_COMPUTE
#include includes/Lighting.glsl
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

float3 GetAmbientColor(float3 position, float3 direction, float3 viewdir)
{
    float anisotropy = GetLightAnisotropy(viewdir, direction, pk_Volume_Anisotropy);
    float4 scenegi = GI_ConeTrace_Volumetric(position);
    float3 staticgi = SampleEnvironment(OctaUV(direction), 1.0f) * anisotropy;
    return staticgi * scenegi.a + scenegi.rgb;
}

layout(local_size_x = PK_W_ALIGNMENT_4, local_size_y = PK_W_ALIGNMENT_4, local_size_z = PK_W_ALIGNMENT_4) in;
void main()
{
    const uint3 id = gl_GlobalInvocationID;
    const float2 uv = (id.xy + 0.5f.xx) / VOLUME_SIZE_XY;

    // Triangle dither range is -1.5 - 1.5 and due to trilinear interpolation we also need to have 2 texels worth of coverage.
    const float zmin = GetVolumeCellDepth(id.z - 3.0f);
    const float zmax = SampleMaxZ(int2(id.xy), 3);

    float3 bluenoise = GetVolumeCellNoise(id);

    float depth = GetVolumeCellDepth(id.z + NoiseUniformToTriangle(bluenoise.x));

    // Texel is inside dither & trilinear interpolation range. Let's clamp it so that we can avoid light leaking through thin surfaces.
    if (zmin < zmax)
    {
        depth = min(zmax, depth);
    }

    depth = max(pk_ProjectionParams.x, depth);

    float3 worldpos = mul(pk_MATRIX_I_V, float4(ClipUVToViewPos(uv, depth), 1.0f)).xyz;

    float3 viewdir = normalize(worldpos - pk_WorldSpaceCameraPos.xyz);

    float3 color = GetAmbientColor(worldpos, normalize(bluenoise - 0.5f + float3(0, 1, 0)), viewdir);

    LightTile tile = GetLightTile(GetTileIndexUV(uv, depth));

    for (uint i = tile.start; i < tile.end; ++i)
    {
        color += GetVolumeLightColor(i, worldpos, viewdir, tile.cascade, pk_Volume_Anisotropy);
    }

    float density = Density(worldpos);

    float4 preval = tex2D(pk_Volume_InjectRead, ReprojectWorldToCoord(worldpos));
    float4 curval = float4(pk_Volume_Intensity * density * color, density);

    curval = lerp(preval, curval, VOLUME_ACCUMULATION);
    curval.a = VOLUME_MIN_DENSITY + curval.a;

    imageStore(pk_Volume_Inject, int3(id), curval);
}
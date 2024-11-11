
#pragma pk_material_property float4 _Color
#pragma pk_material_property float4 _EmissionColor

#define BxDF_ENABLE_SUBSURFACE
#define BxDF_ENABLE_CLEARCOAT
#define SURF_USE_VERTEX_FUNCTION
#include "includes/Noise.glsl"
#include "includes/SurfaceShaderBase.glsl"

float3 GerstnerWave(float4 wave, float3 p, inout float3 tangent, inout float3 binormal)
{
    float steepness = wave.z;
    float wavelength = wave.w;
    float k = 2 * PK_PI / wavelength;
    float c = sqrt(9.8 / k);
    float2 d = normalize(wave.xy);
    float f = k * (dot(d, p.xz) - c * pk_Time.y);
    float a = steepness / k;

    tangent += float3
        (
            -d.x * d.x * (steepness * sin(f)),
            d.x * (steepness * cos(f)),
            -d.x * d.y * (steepness * sin(f))
            );
    binormal += float3
        (
            -d.x * d.y * (steepness * sin(f)),
            d.y * (steepness * cos(f)),
            -d.y * d.y * (steepness * sin(f))
            );
    return float3
        (
            d.x * (a * cos(f)),
            a * sin(f),
            d.y * (a * cos(f))
            );
}

void SURF_FUNCTION_VERTEX(inout SurfaceVaryings surf)
{
    float4 wavea = float4(1, 0, 0.4f, 10);
    float4 waveb = float4(1, 1, 0.1f, 3);
    float4 wavec = float4(-1, 1, 0.2f, 5);

    float3 gridPoint = surf.worldpos.xyz;
    float3 tangent = float3(1, 0, 0);
    float3 binormal = float3(0, 0, 1);
    float3 p = gridPoint;

    p += GerstnerWave(wavea, gridPoint, tangent, binormal) * 0.8f;
    p += GerstnerWave(waveb, gridPoint, tangent, binormal) * 0.8f;
    p += GerstnerWave(wavec, gridPoint, tangent, binormal) * 0.8f;

    surf.normal = normalize(cross(binormal, tangent));

    surf.worldpos.xyz = p;

#if defined(PK_META_PASS_GIVOXELIZE)
    surf.worldpos.y -= 1.0f;
#endif
}

void SURF_FUNCTION_FRAGMENT(float2 uv, inout SurfaceData surf)
{
    float yorigin = pk_ObjectToWorld[2].w;

    float3 noise;
    noise.xy = NoiseCell(int2(surf.worldpos.xz * 8.0f + surf.worldpos.yy * 30.0f));
    noise.z = 1.0f;

    surf.normal = normalize(SURF_MESH_NORMAL + noise * 0.15f);

    float nv = saturate(1.0f - dot(surf.normal, surf.viewdir));

    nv = pow4(nv);

    float depth = unlerp_sat(yorigin - 1.0f, yorigin + 5.0f, surf.worldpos.y);

    surf.albedo = lerp(float3(0, 0.01f, 0.02f), float3(0.3, 0.6, 1.0f), depth * depth);
    surf.albedo = lerp(surf.albedo, float3(0.25f, 0.5f, 0.8f), nv); //PK_ACCESS_INSTANCED_PROP(_Color).xyz;
    surf.alpha = 1.0f;

    surf.subsurface = 0.8f.xxx * depth;

    surf.metallic = max(0.0f, noise.y * 0.5f);
    surf.roughness = max(0.0f, noise.x * 0.2f);
    surf.occlusion = 1.0f;
    surf.clearCoatGloss = 0.5f;
    surf.clearCoat = 1.0f;
}

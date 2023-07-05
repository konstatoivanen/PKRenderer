#version 460
#MaterialProperty float4 _Color
#MaterialProperty float _Metallic
#MaterialProperty float _Roughness
#MaterialProperty float _Occlusion
#MaterialProperty float _NormalAmount
#MaterialProperty texture2D _AlbedoTexture
#MaterialProperty texture2D _PBSTexture
#MaterialProperty texture2D _NormalMap
#define PK_NORMALMAPS
#define PK_HEIGHTMAPS
#include includes/SurfaceShaderBase.glsl

#pragma PROGRAM_VERTEX
void PK_SURFACE_FUNC_VERT(inout SurfaceFragmentVaryings surf) {}

#pragma PROGRAM_FRAGMENT


float4 SampleTriplanar(sampler2D tex, float3 normal, float3 position, float scale)
{
    float3 blend = abs(normal);

    blend /= dot(blend, 1.0.xxx);

    float4 cx = PK_SURF_TEX(tex, position.yz * scale);
    float4 cy = PK_SURF_TEX(tex, position.xz * scale);
    float4 cz = PK_SURF_TEX(tex, position.xy * scale);

    return cx * blend.x + cy * blend.y + cz * blend.z;
}

float3 SampleNormalTriplanar(in SurfaceFragmentVaryings varyings, inout SurfaceData surf, float scale)
{
    float3 blend = abs(PK_SURF_MESH_NORMAL);
    blend /= dot(blend, 1.0.xxx);
    float3 cx = PK_SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, surf.worldpos.yz * scale);
    float3 cy = PK_SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, surf.worldpos.xz * scale);
    float3 cz = PK_SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, surf.worldpos.xy * scale);
    return normalize(cx * blend.x + cy * blend.y + cz * blend.z);
}

void PK_SURFACE_FUNC_FRAG(in SurfaceFragmentVaryings varyings, inout SurfaceData surf)
{
    float2 uv = varyings.vs_TEXCOORD0;
    float3 textureval = SampleTriplanar(_PBSTexture, PK_SURF_MESH_NORMAL, surf.worldpos, 0.25f).xyz;
    surf.metallic = textureval.SRC_METALLIC * _Metallic;
    surf.roughness = textureval.SRC_ROUGHNESS * _Roughness;
    surf.occlusion = lerp(1.0f, textureval.SRC_OCCLUSION, _Occlusion);
    surf.normal = SampleNormalTriplanar(varyings, surf, 0.25f);
    surf.albedo = SampleTriplanar(_AlbedoTexture, PK_SURF_MESH_NORMAL, surf.worldpos, 0.25f).rgb * _Color.xyz;
}
#version 460
#MaterialProperty float4 _Color
#MaterialProperty float _Metallic
#MaterialProperty float _Roughness
#MaterialProperty float _Occlusion
#MaterialProperty float _NormalAmount
#MaterialProperty texture2D _AlbedoTexture
#MaterialProperty texture2D _PBSTexture
#MaterialProperty texture2D _NormalMap

/*
@TODO use separate samplers for these textures. they take a lot of register space by having them combined.
layout(set = 0, binding = 0) uniform sampler albedoSampler;
layout(set = 0, binding = 1) uniform texture2D albedo[];

texture(sampler2D(albedo[material.albedoTextureIndex], albedoSampler), uv);
*/

#define BxDF_ENABLE_SHEEN
#define BxDF_ENABLE_CLEARCOAT
#define PK_USE_TANGENTS
#include includes/SurfaceShaderBase.glsl

#pragma PROGRAM_VERTEX
void PK_SURFACE_FUNC_VERT(inout SurfaceFragmentVaryings surf) {}

#pragma PROGRAM_FRAGMENT

float4 SampleTriplanar(texture2D tex, float3 normal, float3 position, float scale)
{
    float3 blend = abs(normal);
    blend /= dot(blend, 1.0.xxx);
    const float4 cx = PK_SURF_TEX(tex, position.yz * scale);
    const float4 cy = PK_SURF_TEX(tex, position.xz * scale);
    const float4 cz = PK_SURF_TEX(tex, position.xy * scale);
    return cx * blend.x + cy * blend.y + cz * blend.z;
}

float3 SampleNormalTriplanar(in SurfaceFragmentVaryings varyings, inout SurfaceData surf, float scale)
{
    float3 blend = abs(PK_SURF_MESH_NORMAL);
    blend /= dot(blend, 1.0.xxx);
    const float3 cx = PK_SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, surf.worldpos.yz * scale);
    const float3 cy = PK_SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, surf.worldpos.xz * scale);
    const float3 cz = PK_SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, surf.worldpos.xy * scale);
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
    surf.sheen = 1.0f.xxx;
    surf.sheenTint = 0.0f;
    surf.clearCoat = 0.5f.xxx;
    surf.clearCoatGloss = 0.0f;
}
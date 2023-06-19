#version 460
#MaterialProperty float4 _Color
#MaterialProperty float _Metallic
#MaterialProperty float _Roughness
#MaterialProperty float _Occlusion
#MaterialProperty float _NormalAmount
#MaterialProperty float _HeightAmount
#MaterialProperty texture2D _AlbedoTexture
#MaterialProperty texture2D _PBSTexture
#MaterialProperty texture2D _NormalMap
#MaterialProperty texture2D _HeightMap
#define PK_NORMALMAPS
#define PK_HEIGHTMAPS
#include includes/SharedSurfaceShading.glsl

#pragma PROGRAM_VERTEX
void PK_SURFACE_FUNC_VERT(inout SurfaceFragmentVaryings surf) {}

#pragma PROGRAM_FRAGMENT
void PK_SURFACE_FUNC_FRAG(in SurfaceFragmentVaryings varyings, inout SurfaceData surf)
{
    float2 uv = varyings.vs_TEXCOORD0 + PK_SURF_SAMPLE_PARALLAX_OFFSET(_HeightMap, _HeightAmount);
    float3 textureval = PK_SURF_TEX(_PBSTexture, uv).xyz;
    surf.metallic = textureval.SRC_METALLIC * _Metallic;
    surf.roughness = textureval.SRC_ROUGHNESS * _Roughness;
    surf.occlusion = lerp(1.0f, textureval.SRC_OCCLUSION, _Occlusion);
    surf.normal = PK_SURF_SAMPLE_NORMAL(_NormalMap, _NormalAmount, uv);
    surf.albedo = PK_SURF_TEX(_AlbedoTexture, uv).rgb * _Color.rgb;

    // Add glitter
    float t = GlobalNoiseBlueUV(uv).r;
    t = pow5(t);
    t -= 0.875f;
    t *= 50.0f;
    t = max(0.0f, t);
    surf.roughness -= t;
};